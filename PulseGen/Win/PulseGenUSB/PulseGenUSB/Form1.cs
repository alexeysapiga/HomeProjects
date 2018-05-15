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
        Slot[] _slots;
        System.Timers.Timer aTimer = new System.Timers.Timer();
        SerialPort currentPort = new SerialPort();
        private delegate void updateDelegate(string txt);
        private Thread searchArduino;
        public Form1()
        {
            InitializeComponent();
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

        private void buttonSend_Click(object sender, EventArgs e)
        {            
            foreach (var slot in _slots)
            {
                slot.OnWrite();
            }
            if (_dataSender != null)
            {
                _dataSender.Send();
            }
        }


        public class SimpleData
        {
            public SimpleData(int val, byte command) { Value = val; _command = command; }
            public int Value { get; set; }
            public void OnWrite()
            {
                var val = Math.Max(Math.Min(Value, 65535), 0);
                var first = (byte)(val >> 8);
                var second = (byte)(val & 0xFF);
                if (_dataSender != null)
                {
                    _dataSender.Add(_command, first, second);
                }
            }

            byte _command;
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
            }

            public void Add(byte command, byte first, byte second)
            {
                _buffer.Add(command);
                _buffer.Add(first);
                _buffer.Add(second);
            }

            public void Add(byte command, byte first)
            {
                _buffer.Add(command);
                _buffer.Add(first);
            }

            public void Send()
            {
                _port.Write(_buffer.ToArray(), 0, _buffer.Count);
                _buffer.Clear();
            }

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
                IPulse = new SimpleData(0, 0x01);
                IIPulse = new SimpleData(0, 0x02);
                IIIPulse = new SimpleData(0, 0x03);
                I_IIDistance = new SimpleData(0, 0x04);
                I_IIIDistance = new SimpleData(0, 0x05);
                Period = new SimpleData(0, 0x06);
                Strobe = new SimpleData(0, 0x07);
            }
        }

        private void ConfigBoard()
        {
            if (_dataSender == null)
            {
                return;
            }
            //DD9
            _dataSender.Add(0xB, 0x80);
            //DD6
            _dataSender.Add(0x8, 0, 1);
            _dataSender.Add(0x8, 1, 1);
            _dataSender.Add(0x8, 2, 1);
            //DD7
            _dataSender.Add(0x9, 0, 1);
            _dataSender.Add(0x9, 1, 2);
            _dataSender.Add(0x9, 2, 1);
            //DD8
            _dataSender.Add(0xA, 0, 1);
            _dataSender.Add(0xA, 1, 2);
            _dataSender.Add(0xA, 2, 1);
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
                _savedTextBox.Text = FromIntToString(Value);
                Value = _data.Value;
            }

            public void OnWrite()
            {
                _data.Value = Value;
                Value = _data.Value;
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
                if (e.KeyCode == Keys.Enter)
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
