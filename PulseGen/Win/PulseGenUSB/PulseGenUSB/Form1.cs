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
using System.IO;

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
            if (_dataSender != null)
            {
                _dataSender.Add(0xD, (byte)~_currentOutputPins, 0);
            }
        }

        private void AddEnableOutputCommandPeriod(bool check)
        {
            if (_dataSender != null)
            {
                _dataSender.Add(0xD, (byte)(check ? 0xFF : 0x00), 1);
            }
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
            if (_dataSender != null)
            {
                _dataSender.Send();
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            _slots = new Slot[] { new Slot("I, мкс", 10.0, _data.IPulse),
                    new Slot("II, мкс", 10.0, _data.IIPulse),
                    new Slot("III, мкс", 10.0, _data.IIIPulse),
                    new SlotPeriod("T", 1.0, _data.Period),
                    new Slot("I_II, мкс", 1.0, _data.I_IIDistance),
                    new Slot("I_III, мкс", 1.0, _data.I_IIIDistance),
                    new Slot("I_Строб, мкс", 1.0, _data.StrobePos),
                    new Slot("Строб, мкс", 10.0, _data.Strobe)};
            _logRichBox = richTextBox1;

            _slots[0].Bind(label1, button1, button2, textBox1);
            _slots[1].Bind(label2, button3, button4, textBox2);
            _slots[2].Bind(label3, button5, button6, textBox3);
            ((SlotPeriod)_slots[3]).Bind(label4, button7, button8, textBox4, comboBox1);
            _slots[4].Bind(label5, button9, button10, textBox5);
            _slots[5].Bind(label6, button11, button12, textBox6);
            _slots[6].Bind(label7, button13, button14, textBox7);
            _slots[7].Bind(label8, button15, button16, textBox8);
            searchArduino = new Thread(SearchForPort);
            searchArduino.Start();
            comboBox1.SelectedIndex = 2;
            _currentOutputPins = Properties.Settings.Default.PortOutput;
        }

        private delegate void StringArgReturningVoidDelegate(string text);
        private delegate void OnConnectDelegate();
        private delegate void OnDisConnectDelegate();

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
                System.Threading.Thread.Sleep(400);
                CheckItemsOnPins();
                System.Threading.Thread.Sleep(400);
                foreach (var slot in _slots)
                {
                    slot.OnWrite();
                }
                _dataSender.Add(0xD, 0xFF, 1);
                _dataSender.Send();
            }
        }

        private void OnDisConnectMainThread()
        {
            // InvokeRequired required compares the thread ID of the  
            // calling thread to the thread ID of the creating thread.  
            // If these threads are different, it returns true.  
            if (StatusLabel.InvokeRequired)
            {
                OnDisConnectDelegate d = new OnDisConnectDelegate(OnDisConnectMainThread);
                this.Invoke(d, null);
            }
            else
            {
               
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
                OnDisConnectMainThread();                
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
            aTimer = null;

            currentPort.Close();
            searchArduino.Abort();
            searchArduino = null;
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
            public SimpleData StrobePos { get; set; }
            public PulseData()
            {
                IPulse = new SimpleData(Properties.Settings.Default.I, 0x01);
                IIPulse = new SimpleData(Properties.Settings.Default.II, 0x02);
                IIIPulse = new SimpleData(Properties.Settings.Default.III, 0x03);
                I_IIDistance = new SimpleData(Properties.Settings.Default.I_II, 0x04);
                I_IIIDistance = new SimpleData(Properties.Settings.Default.I_III, 0x05);
                Period = new PeriodData(Properties.Settings.Default.T, 0x06, 0x07);
                Strobe = new SimpleData(Properties.Settings.Default.St, 0x09);
                StrobePos = new SimpleData(Properties.Settings.Default.I_St, 0x08);
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

            _dataSender.Add(0xD, (byte)~_currentOutputPins, 0);
            _dataSender.Send();
        }

        public class Slot
        {
            public Slot()
            {
                _savedTextBox = null;
                _savedLabel = null;
                _labelString = null;
                _divider = 0.0;
                _value = 0;
                _data = null;
            }
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
                textBox.LostFocus += new EventHandler(OnFocusChanged);
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
                        _savedTextBox.BackColor = Color.Red;                     
                    }
                    else
                    {
                        _savedTextBox.BackColor = Color.White;
                    }
                    _value = value;
                }
            }

            
            protected virtual void OnInc(object sender, EventArgs e)
            {
                Value += 1;
                _savedTextBox.Text = FromIntToString(Value);
            }

            protected virtual void OnDec(object sender, EventArgs e)
            {
                Value -= 1;
                if (Value < 0)
                {
                    Value = 0;
                }
                _savedTextBox.Text = FromIntToString(Value);
            }

            private void ReadValueFromText(object sender)
            {
                var inpString = ((TextBox)sender).Text;
                inpString = inpString.Replace(".", ",");
                var inpVal = FromStringToValue(inpString);
                Value = inpVal;
                ((TextBox)sender).Text = FromIntToString(inpVal);
            }

            void OnFocusChanged(object sender, EventArgs e)
            {
                ReadValueFromText(sender);
            }

            private void OnEnterValue(object sender, KeyEventArgs e)
            {
                if (e.KeyCode == Keys.Enter)
                {
                    ReadValueFromText(sender);
                }
            }

            protected virtual int FromStringToValue(String inp)
            {
                return (int)(Math.Round(Convert.ToDouble(inp) * _divider));
            }

            protected virtual String FromIntToString(int val)
            {
                return ((double)val / _divider).ToString();
            }

            protected TextBox _savedTextBox;
            protected Label _savedLabel;
            protected String _labelString;
            protected double _divider;
            protected int _value;
            protected SimpleData _data;
        }

        public class SlotPeriod : Slot
        {
            public SlotPeriod(String labelString, double divider, SimpleData data)
            {
                _labelString = labelString;
                _data = data;
                _divider = divider;
            }
            public void Bind(Label label, Button decButton, Button incButton, TextBox textBox, ComboBox comboBox)
            {
                _comboBox = comboBox;
                _comboBox.SelectedIndexChanged += new EventHandler(OnTimeScaleChanged);
                Bind(label, decButton, incButton, textBox);                
            }

            protected override int FromStringToValue(String inp)
            {
                return (int)(Math.Round(Convert.ToDouble(inp) * _divider * GetCoefficient()));
            }

            protected override String FromIntToString(int val)
            {
                return ((double)val / (_divider * GetCoefficient())).ToString();
            }

            protected override void OnInc(object sender, EventArgs e)
            {
                Value += 1 * (int)GetCoefficient();
                _savedTextBox.Text = FromIntToString(Value);
            }

            protected override void OnDec(object sender, EventArgs e)
            {
                Value -= 1 * (int)GetCoefficient();
                if (Value < 0)
                {
                    Value = 0;
                }
                _savedTextBox.Text = FromIntToString(Value);
            }

            private void OnTimeScaleChanged(object sender, EventArgs e)
            {
                _savedTextBox.Text = FromIntToString(Value);
            }

            private double GetCoefficient()
            {
                switch (_comboBox.SelectedIndex)
                {
                    case 0:
                        return 1000000.0;
                    case 1:
                        return 1000.0; 
                    default:
                        return 1.0; 
                }
            }

            private ComboBox _comboBox;
        }

        private void checkedListBox1_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void button17_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.I = _data.IPulse.Value;
            Properties.Settings.Default.II = _data.IIPulse.Value;
            Properties.Settings.Default.III = _data.IIIPulse.Value;
            Properties.Settings.Default.I_II = _data.I_IIDistance.Value;
            Properties.Settings.Default.I_III = _data.I_IIIDistance.Value;
            Properties.Settings.Default.T = _data.Period.Value;
            Properties.Settings.Default.St = _data.Strobe.Value;
            Properties.Settings.Default.I_St = _data.StrobePos.Value;
            Properties.Settings.Default.PortOutput = _currentOutputPins;

            Properties.Settings.Default.Save();
        }
    }
}
