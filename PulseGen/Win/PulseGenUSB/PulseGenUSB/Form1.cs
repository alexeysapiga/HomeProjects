using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using System.Timers;
using System.Threading;

namespace PulseGenUSB
{

    public partial class Form1 : Form
    {
        static ISendDataToNMR _dataSender = null;
        PulseData _data = new PulseData();
        byte _currentOutputPins= 0xFF;
        bool _periodOn = true;
        
        Slot[] _slots;
        System.Timers.Timer aTimer = new System.Timers.Timer();
        SerialPort currentPort = new SerialPort();
        private delegate void updateDelegate(string txt);
        private Thread searchArduino;
        private static RichTextBox _logRichBox;
        public Form1()
        {
            InitializeComponent();
        }

        private void buttonSend_Click(object sender, EventArgs e)
        {            
            foreach (var slot in _slots)
                {
                    slot.OnWrite();
                }
            _dataSender.Add(0xD, 0xFF, 1);
            if (_dataSender != null)
            {
                _dataSender.Send();
            }
        }

        private void AddEnableOutputCommand(byte mask, bool check)
        {
            if (check)
            {
                _currentOutputPins |= mask;
            } else
            {
                _currentOutputPins &= (byte)~mask;
            }
            _dataSender.Add(0xD, (byte)~_currentOutputPins, 0);
        }

        private void AddEnableOutputCommandPeriod(bool check)
        {
            _dataSender.Add(0xD, (byte)(check ?  0xFF : 0x00), 1);

        }
        private void CheckItemsOnPins()
        {
            for (int i = 0; i < 7; i++)
            {
                bool newState = (_currentOutputPins & (1<<i)) > 0 ? true : false;
                
                switch (i) {
                    case 0: checkedListBox1.SetItemChecked(1, newState); break;
                    case 1: checkedListBox1.SetItemChecked(0, newState); break;
                    case 2: checkedListBox1.SetItemChecked(5, newState); break;
                    case 3: checkedListBox1.SetItemChecked(2, newState); break;
                    case 4: checkedListBox1.SetItemChecked(6, newState); break;
                    case 5: checkedListBox1.SetItemChecked(4, newState); break;
                    case 6: checkedListBox1.SetItemChecked(7, newState); break;
                }
            }
            checkedListBox1.SetItemChecked(3, _periodOn);
        }

        private void checkedListBox1_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            bool check = e.NewValue == CheckState.Checked ? true : false;
            switch (e.Index) {
                case 0: AddEnableOutputCommand(1<<1, check); break;
                case 1: AddEnableOutputCommand(1, check); break;
                case 2: AddEnableOutputCommand(1<<3, check); break;
                case 3: AddEnableOutputCommandPeriod(check); break;
                case 4: AddEnableOutputCommand(1<<5, check); break;
                case 5: AddEnableOutputCommand(1<<2, check); break;
                case 6: AddEnableOutputCommand(1<<4, check); break;
                case 7: AddEnableOutputCommand(1<<6, check); break;
            }
            _dataSender.Send();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            _slots = new Slot[] { new Slot("I", 10.0, _data.IPulse),
                    new Slot("II", 10.0, _data.IIPulse),
                    new Slot("III", 10.0, _data.IIIPulse),
                    new Slot("T", 1.0, _data.Period),
                    new Slot("I_II", 1.0, _data.I_IIDistance),
                    new Slot("I_III", 1.0, _data.I_IIIDistance),
                    new Slot("St", 1.0, _data.Strobe) };
            _logRichBox = richTextBox1;

            _slots[0].Bind(label1, button1, button2, textBox1);
            _slots[1].Bind(label2, button3, button4, textBox2);
            _slots[2].Bind(label3, button5, button6, textBox3);
            _slots[3].Bind(label4, button7, button8, textBox4);
            _slots[4].Bind(label5, button9, button10, textBox5);
            _slots[5].Bind(label6, button11, button12, textBox6);
            _slots[6].Bind(label7, button13, button14, textBox7);
            searchArduino = new Thread(SearchForPort);
            searchArduino.Start();
            
        }

        private delegate void StringArgReturningVoidDelegate(string text);
        private delegate void OnConnectDelegate();

        private void SetStatusText(string text)
        {
            // InvokeRequired required compares the thread ID of the  
            // calling thread to the thread ID of the creating thread.  
            // If these threads are different, it returns true.  
            if (StatusLabel.InvokeRequired)
            {
                StringArgReturningVoidDelegate d = new StringArgReturningVoidDelegate(SetStatusText);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                StatusLabel.Text = text;
            }
        }

        private void OnConnectMainThread()
        {
            // InvokeRequired required compares the thread ID of the  
            // calling thread to the thread ID of the creating thread.  
            // If these threads are different, it returns true.  
            if (StatusLabel.InvokeRequired)
            {
                OnConnectDelegate d = new OnConnectDelegate(OnConnectMainThread);
                this.Invoke(d, null);
            }
            else
            {
                ConfigBoard();
                CheckItemsOnPins();
            }
        }



        private void SearchForPort()
        {
            bool ArduinoPortFound = false;
            while (!ArduinoPortFound)
            {
                SetStatusText("Searching");
                try
                {
                    string[] ports = SerialPort.GetPortNames();
                    foreach (string port in ports)
                    {
                        if (port == "COM1")
                        {
                            continue;
                        }
                        currentPort = new SerialPort(port, 9600);
                        currentPort.ReadTimeout = 1000;
                        if (ArduinoDetected())
                        {
                            ArduinoPortFound = true;
                            break;
                        }
                        else
                        {
                            SetStatusText("No board found");
                            ArduinoPortFound = false;
                        }
                    }
                }
                catch
                {
                    SetStatusText("Error");
                    ArduinoPortFound = false;
                }
                if (ArduinoPortFound == false)
                {
                    System.Threading.Thread.Sleep(2000);
                }
            }

            if (ArduinoPortFound == false) return;
            System.Threading.Thread.Sleep(500); // немного подождем

            currentPort.BaudRate = 9600;
            currentPort.DtrEnable = true;
            currentPort.ReadTimeout = 1000;
            try
            {
                currentPort.Open();
            }
            catch { }

            aTimer = new System.Timers.Timer(1000);
            aTimer.Elapsed += OnTimedEvent;
            aTimer.AutoReset = true;
            aTimer.Enabled = true;
            searchArduino.Abort();
        }

        private bool ArduinoDetected()
        {
            try
            {
                if (!currentPort.IsOpen)
                {
                    currentPort.Open();
                }
                System.Threading.Thread.Sleep(1000);
                // небольшая пауза, ведь SerialPort не терпит суеты

                string returnMessage = currentPort.ReadLine();
                currentPort.Close();
                // необходимо чтобы void loop() в скетче содержал код Serial.println("Info from Arduino");
                if (returnMessage.Contains("Info from NMR Pulse Gen"))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            catch (Exception e)
            {
                return false;
            }
        }

        private void OnTimedEvent(object sender, ElapsedEventArgs e)
        {
            if (!currentPort.IsOpen)
            {
                SetStatusText("Disconnected");
                _dataSender = null;
                System.Threading.Thread.Sleep(2000);
                aTimer.Enabled = false;
                searchArduino.Abort();
                searchArduino = new Thread(SearchForPort);
                searchArduino.Start();
                return;
            }
            try // так как после закрытия окна таймер еще может выполнится или предел ожидания может быть превышен
            {
                // удалим накопившееся в буфере
                currentPort.DiscardInBuffer();
                // считаем последнее значение 
                string strFromPort = currentPort.ReadLine();
                //lblPortData.Dispatcher.BeginInvoke(new updateDelegate(updateTextBox), strFromPort);
                SetStatusText("Connected");
                if (_dataSender == null)
                {
                    
                    _dataSender = new SendViaUSB(currentPort);
                    OnConnectMainThread();                    
                }
            }
            catch { }
        }

        

        private void updateTextBox(string txt)
        {
            //lblPortData.Content = txt;
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {

        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            aTimer.Enabled = false;
            currentPort.Close();

        }

       


        public class SimpleData
        {
            public SimpleData() { Value = 0; _command = 0; }
            public SimpleData(int val, byte command) { Value = val; _command = command; }
            public int Value { get; set; }
            public virtual void OnWrite()
            {
                var val = Math.Max(Math.Min(Value, 65535), 0);
                var first = (byte)(val >> 8);
                var second = (byte)(val & 0xFF);
                if (_dataSender != null)
                {
                    _dataSender.Add(_command, first, second);
                }
            }

            protected byte _command;
        }

        public class PeriodData : SimpleData
        {
            public PeriodData(int val, byte commandDivider, byte command)
            {
                _commandDivider = commandDivider;
                _command = command;
                Value = val;                
            }
            public override void OnWrite()
            {
                int divider = Value / 65535;
                if (divider <= 2)
                {
                    divider = 2;
                }
                
                int number = Value / divider;
                
                var first = (byte)(divider >> 8);
                var second = (byte)(divider & 0xFF);
                if (_dataSender != null)
                {
                    _dataSender.Add(_commandDivider, first, second);
                }
                first = (byte)(number >> 8);
                second = (byte)(number & 0xFF);
                if (_dataSender != null)
                {
                    _dataSender.Add(_command, first, second);
                }
            }
            private byte _commandDivider;
        }


        public interface ISendDataToNMR
        {
            void Add(byte command, byte first, byte second);
            void Add(byte command, byte first);
            void Send();            
        }

        public class SendViaUSB : ISendDataToNMR
        {
            public SendViaUSB(SerialPort port)
            {
                _port = port;
                _buffer = new List<byte>();
                _log = new List<string>();
            }

            public void Add(byte command, byte first, byte second)
            {
                _buffer.Add(command);
                _buffer.Add(first);
                _buffer.Add(second);
                _log.Add(command.ToString("X") + " " + first.ToString("X") + " "+ second.ToString("x"));
            }

            public void Add(byte command, byte first)
            {
                _buffer.Add(command);
                _buffer.Add(first);
                _log.Add(command.ToString("X") + " " + first.ToString("X"));
            }

            public void Send()
            {
                _port.Write(_buffer.ToArray(), 0, _buffer.Count);
                
                _logRichBox.Lines = _log.ToArray();
                _buffer.Clear();
               _log.Clear();
            }
            List<string> _log;
            List<byte> _buffer;
            SerialPort _port;
        }


        public class PulseData
        {
            public SimpleData IPulse { get; set; }
            public SimpleData IIPulse { get; set; }
            public SimpleData IIIPulse { get; set; }
            public SimpleData I_IIDistance { get; set; }
            public SimpleData I_IIIDistance { get; set; }
            public SimpleData Period { get; set; }
            public SimpleData Strobe { get; set; }
            public PulseData()
            {
                IPulse = new SimpleData(10, 0x01);
                IIPulse = new SimpleData(30, 0x02);
                IIIPulse = new SimpleData(20, 0x03);
                I_IIDistance = new SimpleData(20, 0x04);
                I_IIIDistance = new SimpleData(40, 0x05);
                Period = new PeriodData(1000, 0x06, 0x07);
                Strobe = new SimpleData(300, 0x08);
            }
        }

        private void ConfigBoard()
        {
            if (_dataSender == null)
            {
                return;
            }
            //DD9
            _dataSender.Add(0xE, 0x80);
            _dataSender.Add(0xD, 0x00, 0);
            _dataSender.Add(0xD, 0x00, 1);
            //DD6
            _dataSender.Add(0xA, 0, 1);
            _dataSender.Add(0xA, 1, 1);
            _dataSender.Add(0xA, 2, 1);
            //DD7
            _dataSender.Add(0xB, 0, 1);
            _dataSender.Add(0xB, 1, 2);
            _dataSender.Add(0xB, 2, 1);
            //DD8
            _dataSender.Add(0xC, 0, 1);
            _dataSender.Add(0xC, 1, 2);
            _dataSender.Add(0xC, 2, 1);

            _dataSender.Add(0x9, 0, 2);
            _dataSender.Send();
        }

        public class Slot
        {
            public Slot(String labelString, double divider, SimpleData data)
            {
                _labelString = labelString;
                _data = data;
                _divider = divider;                
            }

            public void Bind(Label label, Button decButton, Button incButton, TextBox textBox)
            {
                _savedLabel = label;
                _savedTextBox = textBox;
                _savedLabel.Text = _labelString;
                incButton.Click += new EventHandler(OnInc);
                decButton.Click += new EventHandler(OnDec);
                textBox.KeyDown += new KeyEventHandler(OnEnterValue);                
                Value = _data.Value;
                _savedTextBox.Text = FromIntToString(Value);
            }

            public void OnWrite()
            {
                _data.Value = Value;
                Value = _data.Value;
                _data.OnWrite();
            }

            public int Value
            {
                get
                {
                    return _value;
                }
                set
                {
                    if (value != _data.Value)
                    {
                        _savedLabel.Text = "*" + _labelString;
                    }
                    else
                    {
                        _savedLabel.Text = _labelString;
                    }
                    _value = value;
                }
            }

            private void OnInc(object sender, System.EventArgs e)
            {
                Value += 1;
                _savedTextBox.Text = FromIntToString(Value);
            }

            private void OnDec(object sender, System.EventArgs e)
            {
                Value -= 1;
                if (Value < 0)
                {
                    Value = 0;
                }
                _savedTextBox.Text = FromIntToString(Value);
            }

            private void OnEnterValue(object sender, KeyEventArgs e)
            {
                if (e.KeyCode == Keys.Enter || e.KeyCode == Keys.Tab)
                {
                    var inpString = ((TextBox)sender).Text;
                    inpString = inpString.Replace(".", ",");
                    var inpVal = FromStringToValue(inpString);
                    Value = inpVal;
                    ((TextBox)sender).Text = FromIntToString(inpVal);
                }
            }

            private int FromStringToValue(String inp)
            {
                return (int)(Math.Round(Convert.ToDouble(inp) * _divider));
            }

            private String FromIntToString(int val)
            {
                return ((double)val / _divider).ToString();
            }

            private TextBox _savedTextBox;
            private Label _savedLabel;
            private String _labelString;
            private double _divider;
            private int _value;
            private SimpleData _data;
        }

    }
}
