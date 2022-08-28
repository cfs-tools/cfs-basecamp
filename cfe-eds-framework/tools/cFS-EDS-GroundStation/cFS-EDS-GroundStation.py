'''
LEW-20210-1, Python Ground Station for a Core Flight System with CCSDS Electronic Data Sheets Support

Copyright (c) 2020 United States Government as represented by
the Administrator of the National Aeronautics and Space Administration.
All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
'''


'''
cFS-GroundStation Viewer
This module is the display portion of the cFS-GroundStation it consists of two modules
    - Telecommand system which allows the user to send command messages to a core flight instance
    - Telemtry system listens on a particular port and decodes incoming messages
'''
import sys
import time
import os

from PyQt5 import QtGui, QtCore
from PyQt5.QtWidgets import (QApplication, QWidget, QPushButton, QVBoxLayout, QHBoxLayout,
                             QComboBox, QLineEdit, QTextBrowser, QPlainTextEdit,
                             QMessageBox, QLabel)

import GS_Controller
import GS_Model


def ErrorMessage(title, message):
    '''
    Generic error alert
    '''
    msg = QMessageBox()
    msg.setWindowTitle(title)
    msg.setIcon(QMessageBox.Critical)
    msg.setText(message)
    msg.exec_()


def DeleteItemsOfLayout(layout):
    '''
    This function removes the widgets of a QT layout.
    Called when telecommand topics/subcommand are changed
    '''
    if layout is not None:
        while layout.count():
            item = layout.takeAt(layout.count()-1)
            widget = item.widget()
            if widget is not None:
                widget.setParent(None)
            else:
                DeleteItemsOfLayout(item.layout())


class CmdWindow(QWidget):
    '''
    The Command window gives the user all the options to create and send a command message
        - Dropdown menus to select an Instance, Topic and Subcommand
        - Fields to enter payload values
        - A time stamped log showing the commands that were successfully sent
    '''
    def __init__(self):
        super().__init__()

        self.title = "Telecommand System"
        self.top = 300
        self.left = 300
        self.width = 1000
        self.height = 800

        self.instance_name = GS_Model.data.instance_chooser
        self.topic_name = GS_Model.data.topic_chooser
        self.subcommand_name = GS_Model.data.subcommand_chooser

        self.payload_entries = []
        self.payload_values = {}

        self.InitWindow()


    def InitWindow(self):
        '''
        Initialize the Command window layout
        '''
        self.setWindowTitle(self.title)
        self.setGeometry(self.left, self.top, self.width, self.height)

        # Destination IP Address
        self.ip_label = QLabel("IP Address:")
        self.ip_textbox = QLineEdit("127.0.0.1")

        self.hbox_ip = QHBoxLayout()
        self.hbox_ip.addWidget(self.ip_label)
        self.hbox_ip.addWidget(self.ip_textbox)

        # Base UDP Port
        self.port_label = QLabel("Base UDP Port:")
        self.port_textbox = QLineEdit("1234")
        
        self.hbox_port = QHBoxLayout()
        self.hbox_port.addWidget(self.port_label)
        self.hbox_port.addWidget(self.port_textbox)

        # Instance Menu
        self.instance_label = QLabel("Instance:")
        self.instance_menu = QComboBox(self)
        for instance in GS_Model.data.instance_keys:
            self.instance_menu.addItem(instance)
        self.instance_menu.currentIndexChanged.connect(self.UpdateInstance)

        self.hbox_instance = QHBoxLayout()
        self.hbox_instance.addWidget(self.instance_label)
        self.hbox_instance.addWidget(self.instance_menu)

        # Topic Menu
        self.topic_label = QLabel("Topic:")
        self.topic_menu = QComboBox(self)
        for topic in GS_Model.data.telecommand_topic_keys:
            self.topic_menu.addItem(topic)
        self.topic_menu.currentIndexChanged.connect(self.UpdateTopic)

        self.hbox_topic = QHBoxLayout()
        self.hbox_topic.addWidget(self.topic_label)
        self.hbox_topic.addWidget(self.topic_menu)

        # Subcommand Menu
        self.subcommand_label = QLabel("Subcommand:")
        self.subcommand_menu = QComboBox(self)
        for subcommand in GS_Model.data.subcommand_keys:
            self.subcommand_menu.addItem(subcommand)
        self.subcommand_menu.currentIndexChanged.connect(self.UpdateSubcommand)

        self.hbox_subcommand = QHBoxLayout()
        self.hbox_subcommand.addWidget(self.subcommand_label)
        self.hbox_subcommand.addWidget(self.subcommand_menu)

        # Payload Menu
        self.payload_label = QLabel("Payload:")

        self.vbox_payload = QVBoxLayout()
        self.vbox_payload.addWidget(self.payload_label)

        # Send Command Button
        self.send_cmd_button = QPushButton("Send Command")
        self.send_cmd_button.clicked.connect(self.SendCommand)

        # Command Log Display
        self.cmd_log_label = QLabel("Command Log:")
        self.cmd_log = QTextBrowser()

        # Save Command Log Button
        self.save_cmd_log = QPushButton("Save Command Log")
        self.save_cmd_log.clicked.connect(self.SaveCommandLog)

        # Overall Layout
        self.layout = QVBoxLayout()
        self.layout.addLayout(self.hbox_ip)
        self.layout.addLayout(self.hbox_port)
        self.layout.addLayout(self.hbox_instance)
        self.layout.addLayout(self.hbox_topic)
        self.layout.addLayout(self.hbox_subcommand)
        self.layout.addLayout(self.vbox_payload)
        self.layout.addWidget(self.send_cmd_button)
        self.layout.addWidget(self.cmd_log_label)
        self.layout.addWidget(self.cmd_log)
        self.layout.addWidget(self.save_cmd_log)

        self.setLayout(self.layout)

        self.show()


    def UpdateInstance(self):
        '''
        This updates the instance variable when a new instance is selected in the GUI
        '''
        self.instance_name = self.instance_menu.currentText()


    def UpdateTopic(self):
        '''
        This updates the topic variable when a new topic is selected.
        It also updates the Subcommand dropdown and payload entries with new values (if available)
        '''
        self.topic_name = self.topic_menu.currentText()

        if self.topic_name != GS_Model.data.topic_chooser:
            # Update the Subcommand dropdown menu
            GS_Model.data.UpdateSubcommands(self.topic_name)
            self.subcommand_menu.setCurrentIndex(0)
            for i in reversed(range(1, self.subcommand_menu.count())):
                self.subcommand_menu.removeItem(i)

            for subcommand in GS_Model.data.subcommand_keys:
                if subcommand != GS_Model.data.subcommand_chooser:
                    self.subcommand_menu.addItem(subcommand)
            self.subcommand_menu.update()

            # Update payload layout if needed
            eds_id = GS_Controller.control.GetEdsIdFromTopic(self.topic_name)
            payload_struct = GS_Controller.control.GetPayload(eds_id)
            if payload_struct is not None:
                self.UpdatePayload(payload_struct)
            else:
                self.UpdatePayload([])

        else:
            self.subcommand_menu.setCurrentIndex(0)
            for i in reversed(range(1, self.subcommand_menu.count())):
                self.subcommand_menu.removeItem(i)
            self.subcommand_menu.update()

            self.UpdatePayload([])


    def UpdateSubcommand(self):
        '''
        This updates the subcommand variable when a new subcommand is selected in the GUI
        If the new subcommand has new payload values, they are also updated
        '''
        self.subcommand_name = self.subcommand_menu.currentText()
        eds_id = GS_Model.data.subcommand_dict[self.subcommand_name]
        if eds_id != 0:
            payload_struct = GS_Controller.control.GetPayload(eds_id)
            if payload_struct is not None:
                self.UpdatePayload(payload_struct)
            else:
                self.UpdatePayload([])
        else:
            self.UpdatePayload([])


    def ExtractEntries(self, payload_struct):
        '''
        This method generates a list of all the payload entries throughout the payload structure.
        This list is used to update the display with all of the payload entries

        Inputs:
        payload_struct - The payload structure output from GS_Controller.GetPayloadStructure
        '''
        if isinstance(payload_struct, dict):
            for item in list(payload_struct.keys()):
                self.ExtractEntries(payload_struct[item])
        elif isinstance(payload_struct, list):
            for item in payload_struct:
                self.ExtractEntries(item)
        elif isinstance(payload_struct, tuple):
            label = QLabel("{:<30} {}".format(payload_struct[0], payload_struct[1]))
            if payload_struct[2] == 'entry':
                payloadinput = QLineEdit()
            elif payload_struct[2] == 'enum':
                payloadinput = QComboBox()
                for enum_label in list(payload_struct[3].keys()):
                    payloadinput.addItem(enum_label)
            else:
                ErrorMessage("Error", "Something went wrong in the ExtractEntries function")
                return
            self.payload_entries.append((payload_struct[0], payload_struct[1], label, payloadinput))
        else:
            ErrorMessage("Error", "Something went wrong in the ExtractEntries function")


    def UpdatePayload(self, payload_struct):
        '''
        When the payload stucture changes, this method removes the old payload entries
        and adds new ones based on the payload_struct

        Inputs:
        payload_struct - The payload structure output from GS_Controller.GetPayloadStructure
        '''
        # remove the current payload entires in the payload layout (each are their own hbox layouts)
        for i in reversed(range(1, self.vbox_payload.count())):
            layout_item = self.vbox_payload.itemAt(i)
            DeleteItemsOfLayout(layout_item.layout())
            self.vbox_payload.removeItem(layout_item)
        self.vbox_payload.update()

        # add new widgets to represent the new payload entries
        self.payload_entries = []
        self.ExtractEntries(payload_struct)

        for entry in self.payload_entries:
            hbox = QHBoxLayout()
            hbox.addWidget(entry[2])
            hbox.addWidget(entry[3])

            self.vbox_payload.addLayout(hbox)
        self.vbox_payload.update()


    def SendCommand(self):
        '''
        This method takes the input IP Address and payload values and calls
        GS_Controller.control.SendCommand to send the packed message.
        If the command was successfully sent, the command log
        display will be updated with information related to the command.
        '''
        ip_address = self.ip_textbox.text()
        try:
            base_port = int(self.port_textbox.text())
        except ValueError:
            print("Invalid base UDP port: using 1234")
            base_port = 1234
            
        valid_payload = True
        self.payload_values = {}
        for entry in self.payload_entries:
            try:
                value = entry[3].currentText()
            except AttributeError:
                value = entry[3].text()
            if not GS_Controller.ValidPayload(entry, value):
                ErrorMessage("Error", "Invalid input for '{}'".format(entry[0]))
                valid_payload = False
            else:
                self.payload_values[entry[0]] = value

        if valid_payload:
            (cmd_sent, msg, timestamp, port) = GS_Controller.control.SendCommand(ip_address, base_port,
                                                                                 self.instance_name, self.topic_name,
                                                                                 self.subcommand_name, self.payload_values)
            if cmd_sent:
                cmd_log_info = f"Command Sent: {timestamp}\n"
                cmd_log_info += "IP: {}   ".format(ip_address)
                cmd_log_info += "Port: {}\n".format(port)
                cmd_log_info += "Instance: {}   ".format(self.instance_name)
                cmd_log_info += "Topic: {}   ".format(self.topic_name)
                if self.subcommand_name == GS_Model.data.subcommand_chooser:
                    cmd_log_info += "Subcommand: None\n"
                else:
                    cmd_log_info += "Subcommand: {}\n".format(self.subcommand_name)
                cmd_log_payload = ''
                payload_keys = list(self.payload_values.keys())
                for key in payload_keys:
                    cmd_log_payload += "{:<30}: ".format(key)
                    cmd_log_payload += "{}\n".format(self.payload_values[key])
                cmd_log_message = "Data to send: \n{}\n".format(GS_Model.HexString(msg, 8))
                cmd_log_string = cmd_log_info + cmd_log_payload + cmd_log_message + '\n'
                self.cmd_log.append(cmd_log_string)
                self.cmd_log.verticalScrollBar().setValue(self.cmd_log.verticalScrollBar().maximum())
            else:
                ErrorMessage("Error", msg)


    def SaveCommandLog(self):
        '''
        This method outputs the command log to a time stamped text file.
        The command log is also cleared when the log file is written.
        '''
        time_str = time.strftime("%Y-%m-%d__%H_%M_%S", time.gmtime())
        if not os.path.isdir("output/"):
            os.mkdir("output/")
        filename = f"output/Command_Log_{time_str}.txt"     
        fout = open(filename, 'w')
        fout.write(self.cmd_log.toPlainText())
        fout.close()
        self.cmd_log.clear()
        self.cmd_log.update()



class TlmWindow(QWidget):
    '''
    The Telemetry window allows a user to specify a port to listen for telemetry.
    As messages are received, they are decoded and written to the telemetry log.
    Telemetry messages can be written to binary files on an instance:topic basis or all at once.
    '''
    def __init__(self):
        super().__init__()

        self.title = "Telemetry System"
        self.top = 200
        self.left = 200
        self.width = 1000
        self.height = 1000

        self.InitWindow()

        self.tlm_port = None
        self.tlm_thread = None


    def InitWindow(self):
        '''
        Initialize the Telemetry window layout
        '''
        self.setWindowTitle(self.title)
        self.setGeometry(self.left, self.top, self.width, self.height)

        self.port_label = QLabel("Port: ")
        self.port_entry = QLineEdit("1235")
        self.start_tlm_button = QPushButton("Start Listening")
        self.start_tlm_button.clicked.connect(self.StartListening)

        self.tlm_list = QComboBox()
        self.tlm_list.addItem(GS_Model.data.tlm_chooser)
        self.save_tlm_button = QPushButton("Save Telemetry")
        self.save_tlm_button.clicked.connect(self.SaveTlm)
        self.save_all_tlm_button = QPushButton("Save All Telemetry")
        self.save_all_tlm_button.clicked.connect(self.SaveAllTlm)

        self.tlm_log_label = QLabel("Telemetry Log:")
        self.tlm_log = QPlainTextEdit()
        self.tlm_log.setReadOnly(True)
        self.tlm_log.setMaximumBlockCount(2000)

        self.tlm_inputs = QHBoxLayout()
        self.tlm_inputs.addWidget(self.port_label)
        self.tlm_inputs.addWidget(self.port_entry)
        self.tlm_inputs.addWidget(self.start_tlm_button)

        self.tlm_save = QHBoxLayout()
        self.tlm_save.addWidget(self.tlm_list)
        self.tlm_save.addWidget(self.save_tlm_button)
        self.tlm_save.addWidget(self.save_all_tlm_button)

        self.tlm_vbox = QVBoxLayout()

        self.tlm_vbox.addLayout(self.tlm_inputs)
        self.tlm_vbox.addLayout(self.tlm_save)
        self.tlm_vbox.addWidget(self.tlm_log_label)
        self.tlm_vbox.addWidget(self.tlm_log)

        self.setLayout(self.tlm_vbox)
        self.show()


    def StartListening(self):
        '''
        Spawns a thread to start listening to a port for telemetry messages
        '''
        self.tlm_port = self.port_entry.text()
        try:
            port = int(self.tlm_port)
        except TypeError:
            ErrorMessage("Error", "Enter a valid port")
            return
        self.tlm_thread = GS_Controller.TlmListener(port)
        self.tlm_thread.signal.connect(self.UpdateDisplay)
        self.tlm_thread.start()

        self.start_tlm_button.setText("Pause Listening")
        self.start_tlm_button.disconnect()
        self.start_tlm_button.clicked.connect(self.PauseListening)
        self.start_tlm_button.update()


    def PauseListening(self):
        '''
        Pauses telemetry listening
        '''
        self.tlm_thread.PauseListening()
        self.start_tlm_button.setText("Resume Listening")
        self.start_tlm_button.disconnect()
        self.start_tlm_button.clicked.connect(self.ResumeListening)
        self.start_tlm_button.update()


    def ResumeListening(self):
        '''
        Resumes telemetry listening
        '''
        self.tlm_thread.RestartListening()
        self.start_tlm_button.setText("Pause Listening")
        self.start_tlm_button.disconnect()
        self.start_tlm_button.clicked.connect(self.PauseListening)
        self.start_tlm_button.update()


    def SaveTlm(self):
        '''
        Gets the user specified telemetry choice and calls GS_Model.data.SaveTlmType to save
        that type of telemetry
        '''
        tlm_choice = self.tlm_list.currentText()
        if tlm_choice == GS_Model.data.tlm_chooser:
            ErrorMessage("Error", "Please choose a telemetry instance:topic option")
        else:
            GS_Model.data.SaveTlmType(tlm_choice)

    def SaveAllTlm(self):
        '''
        Calls GS_Model.data.SaveAllTlm to save all telemetry types
        '''
        GS_Model.data.SaveAllTlm()

    def UpdateDisplay(self, tlm_type, message):
        '''
        Update the Display based on the received telemetry messages.
        If there is a new telemetry type, add it to the list of telemetry types
        Update the log to show the new telemetry message.

        Inputs:
        tlm_type - Telemetry Instance:Topic identifier
        message - Telemetry message to display
        '''
        if tlm_type != '':
            self.UpdateTlmList(tlm_type)
        self.UpdateTlmDisplay(message)

    def UpdateTlmList(self, tlm_type):
        '''
        Update the telemetry list with the input telemetry type
        '''
        self.tlm_list.addItem(tlm_type)
        self.tlm_list.update()

    def UpdateTlmDisplay(self, message):
        '''
        Update the telemetry log with a new telemetry message
        '''
        self.tlm_log.insertPlainText(message)
        self.tlm_log.verticalScrollBar().setValue(self.tlm_log.verticalScrollBar().maximum())
        self.tlm_log.update()


class MainWindow(QWidget):
    '''
    The primary window for the cFS Groundstation.  This window only contains an entry
    for the mission name and buttons to start the Telecommand and Telemetry Systems
    '''

    def __init__(self):
        super().__init__()

        self.title = "cFS-EDS-GroundStation"
        self.top = 100
        self.left = 100
        self.width = 300
        self.height = 250

        self.InitWindow()

        self.tlm_window = None
        self.cmd_window = None


    def InitWindow(self):
        '''
        Initialize the main cFS-GroundStation window
        '''
        self.setWindowIcon(QtGui.QIcon("nasa.png"))
        self.setWindowTitle(self.title)
        self.setGeometry(self.left, self.top, self.width, self.height)

        # Text box to enter mission name
        self.mission_name = "@CFS_EDS_GS_MISSION_NAME@".lower()
        self.mission_label = QLabel(f"Mission Name:\n{self.mission_name}")
        self.mission_label.setAlignment(QtCore.Qt.AlignCenter)
        self.mission_label.setFont(QtGui.QFont("Courier New", 16))

        # Butten to open Telecommand System
        self.cmd_button = QPushButton("Telecommand\nSystem")
        self.cmd_button.setFont(QtGui.QFont("Courier New", 20))
        self.cmd_button.clicked.connect(self.OpenCmdSystem)

        # Button to open Telemetry System
        self.tlm_button = QPushButton("Telemetry\nSystem")
        self.tlm_button.setFont(QtGui.QFont("Courier New", 20))
        self.tlm_button.clicked.connect(self.OpenTlmSystem)

        # Layout
        vbox = QVBoxLayout()
        vbox.addWidget(self.mission_label)
        vbox.addWidget(self.cmd_button)
        vbox.addWidget(self.tlm_button)
        self.setLayout(vbox)

        self.show()


    def OpenTlmSystem(self):
        '''
        Open the Telemetry System Window
        '''
        if GS_Controller.control.InitializeDatabases(self.mission_name):
            self.tlm_window = TlmWindow()
            self.tlm_window.show()
        else:
            ErrorMessage("Error", "cFS-EDS-GroundStation not properly configured")


    def OpenCmdSystem(self):
        '''
        Open the Telecommand System Window
        '''
        if GS_Controller.control.InitializeDatabases(self.mission_name):
            self.cmd_window = CmdWindow()
            self.cmd_window.show()
        else:
            ErrorMessage("Error", "cFS-EDS-GroundStation not properly configured")


if __name__ == '__main__':
    app = QApplication(sys.argv)
    app.setFont(QtGui.QFont("Courier New"))
    window = MainWindow()
    sys.exit(app.exec())
