import sys
import re
from collections import deque

import pandas as pd
import serial
import serial.tools.list_ports
from PyQt5 import QtWidgets, QtCore
from PyQt5.QtWidgets import QMessageBox, QFileDialog
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.gridspec import GridSpec


class PowerMeterGUI(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Galios matuoklio valdytuvas")

        # mapping for y-axis labels
        self.ylabel_map = {
            'V1': 'Įtampa (V)',
            'V2': 'Įtampa (V)',
            'I1': 'Srovė (mA)',
            'I2': 'Srovė (mA)',
            'P1': 'Galia (mW)',
            'P2': 'Galia (mW)'
        }

        self._init_ui()
        self.serial_port = None
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.read_and_plot)

        # data buffers
        self.data = {
            't': deque(maxlen=2000),
            'V1': deque(maxlen=2000),
            'V2': deque(maxlen=2000),
            'I1': deque(maxlen=2000),
            'I2': deque(maxlen=2000),
            'P1': deque(maxlen=2000),
            'P2': deque(maxlen=2000)
        }

        self.start_time = QtCore.QTime.currentTime()

    def _init_ui(self):
        central = QtWidgets.QWidget()
        layout = QtWidgets.QGridLayout(central)

        # COM port ir baud rate
        layout.addWidget(QtWidgets.QLabel("COM Prievadas:"), 0, 0)
        self.com_combo = QtWidgets.QComboBox()
        self.refresh_com_ports()
        layout.addWidget(self.com_combo, 0, 1)

        layout.addWidget(QtWidgets.QLabel("Baud Rate:"), 0, 2)
        self.baud_combo = QtWidgets.QComboBox()
        for rate in [115200, 57600, 38400, 19200, 9600]:
            self.baud_combo.addItem(str(rate))
        layout.addWidget(self.baud_combo, 0, 3)

        #Intervalai matavimui
        layout.addWidget(QtWidgets.QLabel("Matavimas (ms):"), 1, 0)
        self.measure_combo = QtWidgets.QComboBox()
        self.measure_combo.addItems(["500", "1000", "2000"])
        self.measure_combo.currentIndexChanged.connect(self.update_interval_constraints)
        layout.addWidget(self.measure_combo, 1, 1)

        #Intervalai oledui
        layout.addWidget(QtWidgets.QLabel("OLED atnaujinimas (ms):"), 1, 2)
        self.oled_combo = QtWidgets.QComboBox()
        self.oled_combo.addItems(["500", "1000", "2000"])
        layout.addWidget(self.oled_combo, 1, 3)

        #Intervalai Uartui
        layout.addWidget(QtWidgets.QLabel("PC atnaujinimas (ms):"), 2, 0)
        self.pc_combo = QtWidgets.QComboBox()
        self.pc_combo.addItems(["500", "1000", "2000"])
        layout.addWidget(self.pc_combo, 2, 1)

        #Kiekiai
        layout.addWidget(QtWidgets.QLabel("Kiekis (1):"), 2, 2)
        self.amount1_combo = QtWidgets.QComboBox()
        self.amount1_combo.addItems([str(x) for x in [1,2,4,8,16,32,64]])
        layout.addWidget(self.amount1_combo, 2, 3)

        layout.addWidget(QtWidgets.QLabel("Kiekis (2):"), 3, 0)
        self.amount2_combo = QtWidgets.QComboBox()
        self.amount2_combo.addItems([str(x) for x in [1,2,4,8,16,32,64]])
        layout.addWidget(self.amount2_combo, 3, 1)

        #START ir STOP
        self.start_btn = QtWidgets.QPushButton("START")
        self.start_btn.setStyleSheet("background-color: green; color: white; font-weight: bold;")
        self.start_btn.clicked.connect(self.toggle_start)
        layout.addWidget(self.start_btn, 3, 2)

        #Issaugoti i .xlsx
        self.save_btn = QtWidgets.QPushButton("Išsaugoti")
        self.save_btn.setEnabled(False)
        self.save_btn.clicked.connect(self.save_data)
        layout.addWidget(self.save_btn, 3, 3)

        #Grafikai
        self.fig = Figure(figsize=(6, 8))
        gs = GridSpec(3, 2, figure=self.fig)
        self.fig.subplots_adjust(hspace=0.5, wspace=0.4)
        self.canvas = FigureCanvas(self.fig)

        self.axes = {
            'V1': self.fig.add_subplot(gs[0, 0]),
            'V2': self.fig.add_subplot(gs[0, 1]),
            'I1': self.fig.add_subplot(gs[1, 0]),
            'I2': self.fig.add_subplot(gs[1, 1]),
            'P1': self.fig.add_subplot(gs[2, 0]),
            'P2': self.fig.add_subplot(gs[2, 1]),
        }

        for key, ax in self.axes.items():
            ax.set_title(key + (" (Galia)" if key.startswith('P') else ""))
            ax.set_xlabel('Laikas (s)')
            ax.set_ylabel(self.ylabel_map[key])
            ax.grid(True)

        layout.addWidget(self.canvas, 4, 0, 1, 4)
        self.setCentralWidget(central)

    def refresh_com_ports(self):
        self.com_combo.clear()
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.com_combo.addItems(ports)

    #Jeigu pasirenkam matuoti kas 2000 ms, negali i PC siuntimas vykt kas 1000 ms
    def update_interval_constraints(self):
        min_int = int(self.measure_combo.currentText())
        for combo in (self.oled_combo, self.pc_combo):
            cur = combo.currentText()
            combo.clear()
            for v in [500, 1000, 2000]:
                if v >= min_int:
                    combo.addItem(str(v))
            if cur in [combo.itemText(i) for i in range(combo.count())]:
                combo.setCurrentText(cur)

    def toggle_start(self):
        self.start_btn.setEnabled(False)
        if self.start_btn.text() == "START":
            self.save_btn.setEnabled(False)
            if any(self.data[k] for k in self.data):
                for dq in self.data.values():
                    dq.clear()
                for ax in self.axes.values():
                    ax.clear()
            if not self.setup_serial():
                self.start_btn.setEnabled(True)
                return
            self.send_config()
            self.disable_controls()
            self.start_btn.setText("STOP")
            self.start_btn.setStyleSheet("background-color: red; color: white;")
            self.start_time = QtCore.QTime.currentTime()
            self.timer.start(int(self.pc_combo.currentText()))
        else:
            self.timer.stop()
            #Siunciam STOP i mikroprocesoriu
            stop_msg = "STOPIS1=128;KIEKIS2=128;MATAVIMAS=0500;OLED=1000;PC=2000"
            self.serial_port.write(stop_msg.encode())
            self.serial_port.close()
            self.enable_controls()
            self.start_btn.setText("START")
            self.start_btn.setStyleSheet("background-color: green; color: white;")
            if self.data['t']:
                self.save_btn.setEnabled(True)
        QtCore.QTimer.singleShot(500, lambda: self.start_btn.setEnabled(True))

    def save_data(self):
        path, flt = QFileDialog.getSaveFileName(
            self, "Save Data", "data.xlsx",
            "Excel Files (*.xlsx);;CSV Files (*.csv)"
        )
        if not path:
            return

        df = pd.DataFrame({
            'Laikas, s': list(self.data['t']),
            'V1, V':      list(self.data['V1']),
            'V2, V':      list(self.data['V2']),
            'I1, mA':     list(self.data['I1']),
            'I2, mA':     list(self.data['I2']),
            'P1, mW':     list(self.data['P1']),
            'P2, mW':     list(self.data['P2'])
        }).round(4)

        try:
            if path.endswith('.csv'):
                df.to_csv(path, index=False)
            else:
                df.to_excel(path, index=False)
            QMessageBox.information(self, "Išsaugota", f"Duomenys išsaugoti {path}")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Nepavyko išsaugoti:\n{e}")

    def setup_serial(self):
        port = self.com_combo.currentText()
        baud = int(self.baud_combo.currentText())
        if not port:
            QMessageBox.warning(self, "Error", "Nepasirinktas COM prievadas.")
            return False
        try:
            self.serial_port = serial.Serial(port, baudrate=baud, timeout=1)
            QtCore.QThread.msleep(100)
            return True
        except Exception as e:
            QMessageBox.critical(self, "Serial Error", str(e))
            return False

    def send_config(self):
        k1 = int(self.amount1_combo.currentText())
        k2 = int(self.amount2_combo.currentText())
        mt = int(self.measure_combo.currentText())
        oled = int(self.oled_combo.currentText())
        pc = int(self.pc_combo.currentText())
        msg = f"KIEKIS1={k1:03d};KIEKIS2={k2:03d};MATAVIMAS={mt:04d};OLED={oled:04d};PC={pc:04d}"
        self.serial_port.write(msg.encode())

    def disable_controls(self):
        for w in (
            self.com_combo, self.baud_combo,
            self.measure_combo, self.oled_combo,
            self.pc_combo, self.amount1_combo,
            self.amount2_combo
        ):
            w.setEnabled(False)

    def enable_controls(self):
        for w in (
            self.com_combo, self.baud_combo,
            self.measure_combo, self.oled_combo,
            self.pc_combo, self.amount1_combo,
            self.amount2_combo
        ):
            w.setEnabled(True)

    def handle_remote_stop(self):
        self.timer.stop()
        try:
            self.serial_port.close()
        except:
            pass
        self.enable_controls()
        self.start_btn.setText("START")
        self.start_btn.setStyleSheet("background-color: green; color: white;")
        if self.data['t']:
            self.save_btn.setEnabled(True)

    def read_and_plot(self):
        if not self.serial_port or not self.serial_port.in_waiting:
            return

        line = self.serial_port.readline().decode(errors='ignore').strip()
        if line == "Mygtukas_STOP":
            return self.handle_remote_stop()

        vals = re.findall(r"([VI][12])=\((-?\d+\.\d+)\)", line)
        if vals:
            if self.start_btn.text() == "START":
                self.disable_controls()
                self.start_btn.setText("STOP")
                self.start_btn.setStyleSheet("background-color: red; color: white;")
                self.timer.start(int(self.pc_combo.currentText()))
                self.start_time = QtCore.QTime.currentTime()

            t = self.start_time.msecsTo(QtCore.QTime.currentTime()) / 1000.0
            self.data['t'].append(t)

            for k, v in vals:
                self.data[k].append(float(v))

            if self.data['V1'] and self.data['I1']:
                self.data['P1'].append(self.data['V1'][-1] * self.data['I1'][-1])
            if self.data['V2'] and self.data['I2']:
                self.data['P2'].append(self.data['V2'][-1] * self.data['I2'][-1])

            for key, ax in self.axes.items():
                ax.clear()
                ax.plot(self.data['t'], self.data[key])
                ax.set_title(key)
                ax.set_xlabel('Laikas (s)')
                ax.set_ylabel(self.ylabel_map[key])
                ax.grid(True)

            self.fig.tight_layout(pad=2.0)
            self.canvas.draw()


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    gui = PowerMeterGUI()
    gui.show()
    sys.exit(app.exec_())
