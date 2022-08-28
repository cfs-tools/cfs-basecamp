#!/usr/bin/env python
"""
    Copyright 2022 bitValence, Inc.
    All Rights Reserved.

    This program is free software; you can modify and/or redistribute it
    under the terms of the GNU Affero General Public License
    as published by the Free Software Foundation; version 3 with
    attribution addendums as found in the LICENSE.txt.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    Purpose:
        Provide classes that manage ground and flight files through a GUI. This
        includes displaying directory listing, indiviual file manipulation, and
        transferring files. 
    
    TODO - Create consistent user input validation strategy and fsw tlm status checking
"""

import sys
import time
import os
import socket
import configparser
from queue import Queue
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    sys.path.append('..')
    from cfeconstants  import Cfe
    from telecommand   import TelecommandScript
    from telemetry     import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
    from cmdtlmprocess import CmdTlmProcess
else:
    from .cfeconstants  import Cfe
    from .telecommand   import TelecommandScript
    from .telemetry     import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
    from .cmdtlmprocess import CmdTlmProcess
from tools import crc_32c, compress_abs_path, TextEditor

import PySimpleGUI as sg
    
###############################################################################

class GroundDir():
    """
    """
    def __init__(self, path, sg_window: sg.Window):
        self.sg_window  = sg_window
        self.path = compress_abs_path(path if os.path.isdir(path) else "")
        self.file_list = []
        self.file_list_fmt_str = "{:<20} {:>8}  {:>20}"

    def path_filename(self, filename):
        return os.path.join(self.path, filename)

    def create_file_list(self, path=None):
        if path is not None:
            self.path = compress_abs_path(path)
        dir_list = []
        try:
            dir_list = os.listdir(self.path)
        except:
            pass
        
        # Filenames only: self.file_list = [f for f in dir_list if os.path.isfile(os.path.join(self.path, f))]
        self.file_list = []
        for f in dir_list:
            path_f = os.path.join(self.path, f)
            if os.path.isfile(path_f):
                file_stat = os.stat(path_f) 
                file_text = self.file_list_fmt_str.format(f, file_stat.st_size, time.ctime(file_stat.st_mtime))
                self.file_list.append(file_text)

        if len(self.file_list) == 0:
            self.file_list = ['Folder empty or only contains other folders']
        self.file_list.sort() 
        self.sg_window.update(self.file_list)
        
    def delete_file(self, filename):
        file_pathname = os.path.join(self.path, filename)
        if os.path.exists(file_pathname):
            os.remove(file_pathname)
        else:
            print("TODO: The file does not exist")
        self.create_file_list(self.path)
        
    def rename_file(self, src_file, dst_file):
        src_file_pathname = os.path.join(self.path, src_file)
        if os.path.exists(src_file_pathname):
            dst_file_pathname = os.path.join(self.path, dst_file)
            os.rename(src_file_pathname, dst_file_pathname)
            self.create_file_list(self.path)
        else:
            print("TODO: The file does not exist")
            

###############################################################################

class FlightDir():
    """
    """
    def __init__(self, path, cmd_tlm_process: CmdTlmProcess, sg_window: sg.Window):
        self.cmd_tlm_process = cmd_tlm_process
        self.sg_window = sg_window
        
        self.path = path
        self.file_cnt  = 0
        self.file_list = []
        self.file_list_fmt_str = "{:<20} {:>8}  {:>8}"
        
    def path_filename(self, filename):
        return self.path + '/' + filename
    
    def create_file_list(self, path=None):
        if path is not None:
            self.path = path
        self.file_list = []
        self.filename_max_len = 0
        self.cmd_tlm_process.send_cfs_cmd('FILE_MGR', 'SendDirTlm',  {'DirName': self.path, 'IncludeSizeTime': 1})
        time.sleep(1.5) # Give time for telemetry
        if len(self.file_list) == 0:
            self.sg_window.update(['Check cFS connection or empty/nonexistent directory'])

    def move_up(self):
        """
        '/cf' is the base dir. split('/') will create a first element of "".
        """
        path_list = self.path.split('/')
        print("path_list = " + str(path_list))
        if len(path_list) > 2: 
            new_path = ''
            for dir_name in path_list[1:len(path_list)-1]:
                new_path += '/'+ dir_name 
            self.create_file_list(new_path)
            
    def move_down(self, dir_name):
        """
        
        """
        dir_path = self.path_filename(dir_name)
        print('dir_path = ' + dir_path)
        self.create_file_list(dir_path)

    def create_dir(self, dir_name):
        print('self.path = ' +  self.path)        
        dir_path = self.path_filename(dir_name)
        print('dir_path = ' +  dir_path)
        self.cmd_tlm_process.send_cfs_cmd('FILE_MGR', 'CreateDir',  {'DirName': dir_path})
        self.create_file_list(dir_path)

    def delete_dir(self, dir_name):
        dir_name = self.path_filename(dir_name)
        self.cmd_tlm_process.send_cfs_cmd('FILE_MGR', 'DeleteDir',  {'DirName': dir_name})
        self.create_file_list(self.path)

    def delete_file(self, filename):
        filename = self.path_filename(filename)
        self.cmd_tlm_process.send_cfs_cmd('FILE_MGR', 'DeleteFile',  {'Filename': filename})
        self.create_file_list(self.path)

    def rename_file(self, src_file, dst_file):
        src_file =  self.path_filename(src_file)
        dst_file =  self.path_filename(dst_file)
        self.cmd_tlm_process.send_cfs_cmd('FILE_MGR', 'RenameFile',  {'SourceFilename': src_file, 'TargetFilename': dst_file})
        self.create_file_list(self.path)

    def filemgr_dir_list_callback(self, time, payload):
        print("payload.DirName = "       + str(payload.DirName))
        print("payload.DirFileCnt = "    + str(payload.DirFileCnt))
        print("payload.PktFileCnt = "    + str(payload.PktFileCnt))
        print("payload.DirListOffset = " + str(payload.DirListOffset))        
        for entry in payload.FileList:
            if (len(str(entry['Name'])) > 0):
                file_text = self.file_list_fmt_str.format(str(entry['Name']), str(entry['Size']), str(entry['Time']))
                self.file_list.append(file_text)
        #print("file_list: " + str(self.file_list))
        self.sg_window.update(self.file_list)


###############################################################################

class FileXfer():
    """
    """
    def __init__(self, cmd_tlm_process: CmdTlmProcess):
        self.cmd_tlm_process = cmd_tlm_process

        self.recv_state = 'IDLE'
        self.recv_file  = None
        self.recv_flt_filename = None
        self.recv_gnd_filename = None
        self.gnd_file_list_refresh = None  # Callback function to refresh ground display

    def send_file(self, gnd_file, flt_file):
        """
        Send a file to the cFS. This is a prototype 
        TODO - General to binary once the EDS binary block updates are approved
        TODO - and implemented in cfe-eds-framework 
        """
        file_crc    = 0
        bytes_read  = 0
        data_seg_id = 1
        file_len = os.stat(gnd_file).st_size
                
        #TODO sg.popup("Before SendFile command", title='FILE_XFER Debug', grab_anywhere=True, modal=False)
        self.cmd_tlm_process.send_cfs_cmd('FILE_XFER', 'SendFile', {'DestFilename': flt_file})

        # https://stackoverflow.com/questions/52722787/problem-sending-binary-files-via-sockets-python
        # send file size as big endian 64 bit value (8 bytes)
        # self.sock.sendall(os.stat(gnd_file).st_size.tobytes(8,'big'))
        with open(gnd_file,'r') as f: 
            while True:
                data_segment = f.read(Cfe.FILE_XFER_DATA_SEG_LEN) #bytearray(f.read(64))
                if not data_segment: # Null indicates EOF
                    break
                
                #TODO sg.popup("Before SendFitpDataSegment command", title='FILE_XFER Debug', grab_anywhere=True, modal=False)
                self.cmd_tlm_process.send_cfs_cmd('FILE_XFER', 'SendFitpDataSegment', {'Id': data_seg_id, 'Len': len(data_segment), 'Data': data_segment})
                file_crc = crc_32c(file_crc, bytearray(data_segment,'utf-8'))
                data_seg_id += 1
                time.sleep(0.25)
            #TODO sg.popup("Before FinishFitpTransfer command", title='FILE_XFER Debug', grab_anywhere=True, modal=False)
            self.cmd_tlm_process.send_cfs_cmd('FILE_XFER', 'FinishFitpTransfer', {'FileLen': file_len, 'FileCrc': file_crc, 'LastDataSegmentId': data_seg_id-1})

    def tlm_callback(self, tlm_msg: TelemetryMessage):
        """
        """
        print("filexfer_callback()")
        payload = payload = tlm_msg.payload()
        if tlm_msg.msg_name == 'FOTP_START_TRANSFER_TLM':
            self.recv_state = 'START'
            print('Start receive file for %s with length %d' % (payload.SrcFilename, payload.DataLen))
            self.recv_file = open(self.recv_gnd_filename, "w")
        elif tlm_msg.msg_name == 'FOTP_DATA_SEGMENT_TLM':
            self.recv_state = 'RECV_DATA'
            print('Receive file data segment %d, length %d, data: %s' % (payload.Id, payload.Len, str(payload.Data)))
            self.recv_file.write(str(payload.Data))
        elif tlm_msg.msg_name == 'FOTP_FINISH_TRANSFER_TLM':
            self.recv_state = 'FINISH'
            print('Finish receive file with length %d, CRC %d, Last Data Segment ID %d' % (payload.FileLen, payload.FileCrc, payload.LastDataSegmentId))
            self.recv_file.close()
            self.recv_file = None
            if self.gnd_file_list_refresh is not None:
                self.gnd_file_list_refresh()
                
    def start_recv_file(self, flt_file, gnd_file, gnd_file_list_refresh):
        self.recv_flt_filename = flt_file
        self.recv_gnd_filename = gnd_file
        self.gnd_file_list_refresh = gnd_file_list_refresh
        if self.recv_file is not None:
            self.recv_file.close()
            self.recv_file = None
        if self.recv_state not in ('IDLE', 'FINISH'):
            self.cancel_recv_file()
        self.cmd_tlm_process.send_cfs_cmd('FILE_XFER', 'StartReceiveFile', {'DataSegLen': Cfe.FILE_XFER_DATA_SEG_LEN, 'DataSegOffset': 0, 'SrcFilename': ''.join(flt_file)})


    def cancel_recv_file(self):
        self.cmd_tlm_process.send_cfs_cmd('FILE_XFER', 'CancelSendFile', {})
        self.recv_state = 'IDLE'
            

###############################################################################

class FileBrowserTelemetryMonitor(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, tlm_monitors, event_callback, filemgr_callback, filexfer_callback): 
        super().__init__(tlm_server)

        self.tlm_monitors = tlm_monitors
        self.event_callback = event_callback
        self.filemgr_callback = filemgr_callback
        self.filexfer_callback = filexfer_callback
        
        self.sys_apps = ['CFE_ES', 'CFE_EVS', 'FILE_MGR', 'FILE_XFER']
        
        for msg in self.tlm_server.tlm_messages:
            tlm_msg = self.tlm_server.tlm_messages[msg]
            if tlm_msg.app_name in self.sys_apps:
                self.tlm_server.add_msg_observer(tlm_msg, self)        
                #logger.info("system telemetry adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))
                print("system telemetry adding observer for %s: %s" % (tlm_msg.app_name, tlm_msg.msg_name))
        

    def update(self, tlm_msg: TelemetryMessage) -> None:
        """
        Receive telemetry updates
        """
        #todo: Determine best tlm identification method: if int(tlm_msg.app_id) == int(self.cfe_es_hk.app_id):
        
        if tlm_msg.app_name == 'FILE_MGR':
            if tlm_msg.msg_name == 'DIR_LIST_TLM':
                payload = tlm_msg.payload()
                self.filemgr_callback(str(tlm_msg.sec_hdr().Seconds), payload)
        
        elif tlm_msg.app_name == 'CFE_EVS':
            if tlm_msg.msg_name == 'LONG_EVENT_MSG':
                payload = tlm_msg.payload()
                pkt_id = payload.PacketID
                event_text = "FSW Event at %s: %s, %d - %s" % \
                             (str(tlm_msg.sec_hdr().Seconds), pkt_id.AppName, pkt_id.EventType, payload.Message)
                self.event_callback(event_text)
                
        elif tlm_msg.app_name == 'FILE_XFER':
            if 'FOTP' in tlm_msg.msg_name:
                self.filexfer_callback(tlm_msg)
              
                
###############################################################################

class FileBrowser(CmdTlmProcess):
    """
    Provide a user interface for managing ground and flight directories and
    files. It also supports transferring files between the flight and ground.
    """
    def __init__(self, gnd_path, flt_path, gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout):
        super().__init__(gnd_ip_addr, router_cmd_port, tlm_port, tlm_timeout)
        
        self.default_gnd_path = gnd_path
        self.default_flt_path = flt_path
        self.event_history = ""
        self.init_cycle = True
            
    def event_callback(self, event_txt):
        self.display_event(event_txt)
            
    def update_event_history_str(self, new_event_text):
        time = datetime.now().strftime("%H:%M:%S")
        event_str = time + " - " + new_event_text + "\n"        
        self.event_history += event_str
 
    def display_event(self, new_event_text):
        self.update_event_history_str(new_event_text)
        self.window["-EVENT_TEXT-"].update(self.event_history)
                
    def get_filename(self, gui_filename):
        """
        Get the filename from a GUI file listing. Both ground and flight listing
        start with the filename followed by a space.
        """
        return gui_filename.split(' ')[0]
    
    def gui(self):
        col_title_font = ('Arial bold',20)
        pri_hdr_font   = ('Arial bold',14)
        list_font      = ('Courier',11)
        log_font       = ('Courier',11)
        self.gnd_file_menu = ['_', ['Refresh', '---', 'Send to Flight', 'Edit File', '---',  'Rename File', 'Delete File']] #TODO - Decide on dir support 
        self.gnd_col = [
            [sg.Text('Ground', font=col_title_font)],
            [sg.Text('Folder'), sg.In(self.default_gnd_path, size=(25,1), enable_events=True ,key='-GND_FOLDER-'), sg.FolderBrowse(initial_folder=self.default_gnd_path)],
            [sg.Listbox(values=[], font=list_font, enable_events=True, size=(50,20), key='-GND_FILE_LIST-', right_click_menu=self.gnd_file_menu)]]
        
        # Duplicate ground names have a trailing space to differentiate them. A little kludgy but it works
        self.flt_file_menu = ['_', [ 'Refresh ', '---', 'List Dir', 'Send to Ground', 'Cancel Send', '---',  'Create Dir', 'Delete Dir', '---', 'Rename File ', 'Delete File ']] 
        self.flt_col = [
            [sg.Text('Flight', font=col_title_font)],
            [sg.Text('Folder'), sg.In(self.default_flt_path, size=(25,1), enable_events=True ,key='-FLT_FOLDER-'),
            sg.Button('▲', font='arrow_font 7', border_width=0, pad=(2,0), key='-FLT_UP-')],
            [sg.Listbox(values=[], font=list_font, enable_events=True, size=(50,20), key='-FLT_FILE_LIST-', right_click_menu=self.flt_file_menu)]]

        self.layout = [
            [sg.Column(self.gnd_col, element_justification='c'), sg.VSeperator(), sg.Column(self.flt_col, element_justification='c')],
            [sg.Text('Ground & Flight Events', font=pri_hdr_font), sg.Button('Clear', enable_events=True, key='-CLEAR_EVENTS-', pad=(5,1))],
            [sg.MLine(default_text=self.event_history, font=log_font, enable_events=True, size=(105, 5), key='-EVENT_TEXT-')]]
            
 
        self.window = sg.Window('File Browser', self.layout, resizable=True)
        
        self.flt_dir   = FlightDir(self.default_flt_path, self, self.window['-FLT_FILE_LIST-'])
        self.gnd_dir   = GroundDir(self.default_gnd_path, self.window['-GND_FILE_LIST-'])
        self.file_xfer = FileXfer(self)

        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}, 'FILE_MGR': {'DIR_LIST_TLM': ['Seconds']}}        
        self.tlm_monitor = FileBrowserTelemetryMonitor(self.tlm_server, self.tlm_monitors, self.event_callback, self.flt_dir.filemgr_dir_list_callback, self.file_xfer.tlm_callback)
        self.tlm_server.execute()

        while True:

            self.event, self.values = self.window.read(timeout=100)
        
            if self.init_cycle:
                self.init_cycle = False
                self.gnd_dir.create_file_list(self.default_gnd_path)
                self.flt_dir.create_file_list(self.default_flt_path)
           

            ### Admin ###

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            elif self.event == '-CLEAR_EVENTS-':
                self.event_history = ""
                self.display_event("Cleared event display")


            ### File Transfer ###

            elif self.event == 'Send to Flight':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-GND_FILE_LIST-'][0])
                    gnd_file = self.gnd_dir.path_filename(filename)
                    flt_file = self.flt_dir.path_filename(filename)
                    self.file_xfer.send_file(gnd_file, flt_file)
                    self.flt_dir.create_file_list()
                else:
                    sg.popup("Please select/highlight a file to be transferred to the cFS", title='Send File to FLight', grab_anywhere=True, modal=False)
            
            elif self.event == 'Send to Ground':
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-FLT_FILE_LIST-'][0])
                    flt_file = self.flt_dir.path_filename(filename)
                    gnd_file = self.gnd_dir.path_filename(filename)
                    print('>>>>flt_file: %s, gnd_file: %s' % (flt_file, gnd_file))
                    self.file_xfer.start_recv_file(flt_file, gnd_file, self.gnd_dir.create_file_list)
                    #TODO - Trigger ground file list display refresh
                else:
                    sg.popup("Please select/highlight a file to be transferred to the ground", title='Send File to FLight', grab_anywhere=True, modal=False)
                               
            elif self.event == 'Cancel Send':
                self.file_xfer.cancel_recv_file()

            ### Ground ###
                
            elif self.event == '-GND_FOLDER-':                         
                self.gnd_dir.create_file_list(self.values['-GND_FOLDER-'])

            elif self.event == 'Refresh':                         
                self.gnd_dir.create_file_list()

            elif self.event == 'Edit File':
                """
                Only loads initial file if it is of a partial type. Does allow editor to be 
                launched if no file selected. 
                """
                #TODO - Use ini file config
                cwd = os.getcwd()
                if 'cfsinterface' in cwd:
                    tools_path = compress_abs_path(os.path.join(cwd, "../tools"))
                else:
                    tools_path = os.path.join(cwd, "tools")
                filename = ''
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-GND_FILE_LIST-'][0])
                    if filename.endswith((".txt", ".json", ".h", ".c", ".py", ".cmake", ".scr")):
                        filename = self.gnd_dir.path_filename(filename)
                self.text_editor = sg.execute_py_file("texteditor.py", parms=filename, cwd=tools_path)

            elif self.event == 'Delete File':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                   filename = self.get_filename(self.values['-GND_FILE_LIST-'][0])
                   self.gnd_dir.delete_file(filename)

            elif self.event == 'Rename File':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    dst_file = sg.popup_get_text(title='Rename ' + self.values['-GND_FILE_LIST-'][0], message='Please enter the new filename')
                    if dst_file is not None:
                        filename = self.get_filename(self.values['-GND_FILE_LIST-'][0])
                        self.gnd_dir.rename_file(filename, dst_file)

            ### Flight ###

            elif self.event == '-FLT_FOLDER-':
                self.flt_dir.create_file_list(self.values['-FLT_FOLDER-'])                

            elif self.event == '-FLT_UP-':
                self.flt_dir.move_up()
                self.window['-FLT_FOLDER-'].update(self.flt_dir.path)

            elif self.event == 'Refresh ':                         
                self.flt_dir.create_file_list()

            elif self.event == 'List Dir':
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-FLT_FILE_LIST-'][0])
                    self.flt_dir.move_down(filename)
                    self.window['-FLT_FOLDER-'].update(self.flt_dir.path)

            elif self.event == 'Create Dir':
                dir_name = sg.popup_get_text(title='Create Directory', message='Please enter new directory name')
                if dir_name is not None:
                    self.flt_dir.create_dir(dir_name)
                    self.window['-FLT_FOLDER-'].update(self.flt_dir.path)

            elif self.event == 'Delete Dir':
                """
                Let FileMgr app perform error checks
                """
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-FLT_FILE_LIST-'][0])
                    self.flt_dir.delete_dir(filename)
                    self.window['-FLT_FOLDER-'].update(self.flt_dir.path)

            elif self.event == 'Delete File ':
                """
                Let FileMgr app perform error checks
                """
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-FLT_FILE_LIST-'][0])
                    self.flt_dir.delete_file(filename)
                    self.window['-FLT_FOLDER-'].update(self.flt_dir.path)

            elif self.event == 'Rename File ':
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-FLT_FILE_LIST-'][0])
                    dst_file = sg.popup_get_text(title='Rename '+filename, message='Please enter the new filename')
                    if dst_file is not None:
                        self.flt_dir.rename_file(filename, dst_file)
                        self.window['-FLT_FOLDER-'].update(self.flt_dir.path)
            

    def execute(self):
        self.gui()
    
        
    def shutdown(self):

        self.window.close()       
        self.tlm_server.shutdown()    


###############################################################################

if __name__ == '__main__':

    print(f"Name of the script      : {sys.argv[0]=}")
    print(f"Arguments of the script : {sys.argv[1:]=}")

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    #tlm_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #tlm_socket.sendto("127.0.0.1,1234".encode(),("127.0.0.1",8888))

    cfs_startup_path = config.get('PATHS','CFS_STARTUP_PATH')
    flt_server_path = config.get('PATHS','FLT_SERVER_PATH')
    gnd_path = compress_abs_path(os.path.join(os.getcwd(), '..', flt_server_path))
    cfs_host_addr = config.get('NETWORK','CFS_HOST_ADDR')
    cmd_port      = config.getint('NETWORK','FILE_BROWSER_CMD_PORT')
    tlm_port      = config.getint('NETWORK','FILE_BROWSER_TLM_PORT')
    
    file_browser = FileBrowser(gnd_path, cfs_startup_path, cfs_host_addr, cmd_port, tlm_port, 1.0)
    file_browser.execute()
    
    

