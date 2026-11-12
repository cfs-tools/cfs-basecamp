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
      includes displaying directory listing, individual file manipulation, and
      transferring files. 
    
    Notes:
      1. EDS types need to be converted to Python types. For example, in RecvFile
         payload.Len is converted to an integer using int()
             self.bytes_written = self.bytes_written + int(payload.Len)
         If this isn't done teh following error is reported     
             Payload.Len TypeError: unsupported operand type(s) for +: 'int' and 'EdsLib.DatabaseEntry.basecamp.BASE_TYPES.uint16'
    TODO: Create consistent user input validation strategy and fsw tlm status checking

"""

import sys
sys.path.append("..")
import os
import time
import socket
import configparser
from queue import Queue
from datetime import datetime
import zlib
import queue

import logging
logger = logging.getLogger(__name__)

import FreeSimpleGUI as sg

from tools.bcpaths   import compress_abs_path, BcPathConst, BcPath, BcPathMgr
from tools.utils     import bin_hex_decode, bin_hex_encode

from cfsinterface.cfeconstants  import Cfe
from cfsinterface.telecommand   import TelecommandScript
from cfsinterface.telemetry     import TelemetryMessage, TelemetryObserver, TelemetrySocketServer
from cfsinterface.cmdtlmprocess import CmdTlmProcess

POPUP_EVENT = 'popup_event'

###############################################################################

class GroundDir():
    """
    """
    def __init__(self, gnd_path, gui_dir_queue: Queue, gui_evt_queue: Queue):
        self.gui_dir_queue = gui_dir_queue
        self.gui_evt_queue = gui_evt_queue
        self.path = gnd_path if os.path.isdir(gnd_path) else ""
        self.file_list = []
        if self.path == "":
            self.file_list = [f'Initial ground directory {gnd_path} is not a directory']
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
        self.gui_dir_queue.put(self.file_list)
        
    def delete_file(self, filename):
        file_pathname = os.path.join(self.path, filename)
        if os.path.exists(file_pathname):
            os.remove(file_pathname)
            self.gui_evt_queue.put(f'GND Event: Successfully deleted {file_pathname}')
        else:
            self.gui_evt_queue.put(f'GND Event: Error deleting {file_pathname}, file does not exist')
        self.create_file_list(self.path)
        
    def rename_file(self, src_file, dst_file):
        src_file_pathname = os.path.join(self.path, src_file)
        if os.path.exists(src_file_pathname):
            dst_file_pathname = os.path.join(self.path, dst_file)
            os.rename(src_file_pathname, dst_file_pathname)
            self.gui_evt_queue.put(f'GND Event: Successfully renamed {src_file_pathname} to {dst_file_pathname}')
            self.create_file_list(self.path)
        else:
            self.gui_evt_queue.put(f'GND Event: Error renaming {src_file_pathname}, file does not exist')
            

###############################################################################

class FlightDir():
    """
    """
    
    def __init__(self, path,  gui_dir_queue: Queue, gui_evt_queue: Queue, flt_cmd_queue: Queue):
        self.gui_dir_queue = gui_dir_queue
        self.flt_cmd_queue = flt_cmd_queue
        self.gui_evt_queue = gui_evt_queue
                
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
        self.flt_cmd_queue.put({ 'app': 'FILE_MGR', 'cmd': 'SendDirTlm', 'prm': {'DirName': f'{self.path}', 'IncludeSizeTime': 1}})
        time.sleep(2.0) # Give time for telemetry (FILE_MGR's SEND_DIR_TLM_DELAY impacts this)
        if len(self.file_list) == 0:
            self.gui_dir_queue.put(['Check cFS connection or empty/nonexistent directory'])
        
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
        self.flt_cmd_queue.put({ 'app': 'FILE_MGR', 'cmd': 'CreateDir', 'prm': {'DirName': dir_path}})
        self.create_file_list(dir_path)

    def delete_dir(self, dir_name):
        dir_name = self.path_filename(dir_name)
        self.flt_cmd_queue.put({ 'app': 'FILE_MGR', 'cmd': 'DeleteDir', 'prm': {'DirName': dir_name}})
        self.create_file_list(self.path)

    def delete_file(self, filename):
        filename = self.path_filename(filename)
        self.flt_cmd_queue.put({ 'app': 'FILE_MGR', 'cmd': 'DeleteFile', 'prm': {'Filename': filename}})
        self.create_file_list(self.path)

    def rename_file(self, src_file, dst_file):
        src_file =  self.path_filename(src_file)
        dst_file =  self.path_filename(dst_file)
        self.flt_cmd_queue.put({ 'app': 'FILE_MGR', 'cmd': 'RenameFile', 'prm': {'SourceFilename': src_file, 'TargetFilename': dst_file}})
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
        ##print("file_list: " + str(self.file_list))
        self.file_list.sort()
        self.gui_dir_queue.put(self.file_list)


###############################################################################

class RecvFile():
    """
    This is the ground component of the FSW File Output Transfer Protocol (FOTP).
    """
    # Debug file is designed for a single transfer attempt after file browser is
    # started. Additional logic needs to be added for a more general utility
    DEBUG_FILE  = False
    DEBUG_PRINT = True
    
    def __init__(self, gui_evt_queue: Queue, flt_cmd_queue: Queue):
        
        self.gui_evt_queue = gui_evt_queue
        self.flt_cmd_queue = flt_cmd_queue

        self.xfer_state = 'IDLE'
        self.bytes_written   = 0
        self.last_segment_id = 0
        
        self.file = None
        self.file_crc = 0
        self.file_xfer_cnt = 0
        self.bin_file = False
        self.flt_filename = None
        self.gnd_filename = None
        self.gnd_file_list_refresh = None  # Callback function to refresh ground display
        self.debug('open','')
    
    def debug(self, action, msg):
        if RecvFile.DEBUG_FILE:
            if action == 'open':
                self.debug_file = open('file_browser_debug.txt', 'w')
            elif action == 'write':
                self.debug_file.write(f'{msg}\n')
                if DEBUG_PRINT:
                    print(msg)
            elif action == 'close':
                self.debug_file.close()
                
    def tlm_callback(self, tlm_msg: TelemetryMessage):
        """
        TODO: Verify/enforce packets received match follow state transitions
        """
        self.debug('write', f'RecvFile::tlm_callback(): {tlm_msg.msg_name}')
        payload = tlm_msg.payload()
        if 'START' in tlm_msg.msg_name:
            self.xfer_state = 'START'
            self.bytes_written   = 0
            self.last_segment_id = 0  
            self.debug('write', f'Start receive file for {payload.SrcFilename} with length {payload.DataLen} binary flag {payload.BinFile}')
            self.send_gnd_event(f'Start receive file for {payload.SrcFilename} with length {payload.DataLen} binary flag {payload.BinFile}')
            open_flags = 'w'
            if payload.BinFile:
                open_flags = 'wb'
                self.bin_file = True
            self.file = open(self.gnd_filename, open_flags)
        
        elif 'DATA' in tlm_msg.msg_name:
            self.xfer_state = 'RECV_DATA'
            self.debug('write', f'Received data segment {payload.Id}, length {payload.Len}') # , data: {str(payload.Data)}
            self.debug('write', f'RecvFile state: self.last_segment_id {self.last_segment_id}, self.bytes_written {self.bytes_written}')
            if self.bin_file:
                bin_data_segment = bin_hex_decode(str(payload.Data))
                self.file_crc = zlib.crc32(bin_data_segment, self.file_crc) 
                self.file.write(bin_data_segment)
            else:
                self.file_crc = zlib.crc32(bytearray(payload.Data)[:payload.Len], self.file_crc) 
                self.file.write(str(payload.Data))

            self.bytes_written = self.bytes_written + int(payload.Len)
            self.last_segment_id += 1
            
        elif 'FINISH' in tlm_msg.msg_name:
            """
            See file prologue for EDS type operations
            """
            self.debug('write', f'Finish receive file with length {payload.FileLen}, CRC {payload.FileCrc}, Last Data Segment ID {payload.LastDataSegmentId}')
 
            # Set common portion of status string
            xfer_status = {
                "File Type": "binary" if self.bin_file else "text",
                "CRC": "Valid\n",
                "Length": "Valid\n",
                "Last Segment ID": "Valid\n"
            }
            
            validity_failures = 0
            if (self.file_crc & 0xFFFFFFFF) != int(payload.FileCrc):
                validity_failures += 1
                xfer_status["CRC"] = f'Invalid CRC: Ground value {self.file_crc}, Flight value {hex(payload.FileCrc).upper()}\n'
                self.debug('write', xfer_status["CRC"])
            
            if self.bytes_written != int(payload.FileLen):
                validity_failures += 1
                xfer_status["Length"] = f'Invalid file length: Ground value {self.bytes_written}, Flight value {payload.FileLen}\n'
            
            if self.last_segment_id != int(payload.LastDataSegmentId):
                validity_failures += 1
                xfer_status["Last Segment ID"] = f'Final segment ID miscompare: Ground value {self.last_segment_id}, Flight value {payload.LastDataSegmentId}\n'

            if validity_failures == 0:
                self.file_xfer_cnt += 1
                self.send_gnd_event(f'Successfully received file {self.flt_filename} with length {payload.FileLen}, CRC {hex(payload.FileCrc).upper()}, Last Data Segment ID {payload.LastDataSegmentId}')
            else:
                status_str = f'Error receiving flight {xfer_status["File Type"]} file {self.flt_filename} with the following status:\nCRC: {xfer_status["CRC"]}Length: {xfer_status["Length"]}Last Segment ID: {xfer_status["Last Segment ID"]}'
                self.send_gnd_event(status_str, popup_event=True)
                
            # Reset state variables
            self.xfer_state = 'FINISH'
            self.file = None
            self.bin_file = False
            self.debug('close','')
            if self.gnd_file_list_refresh is not None:
                self.gnd_file_list_refresh()

    def start_xfer(self, flt_file, gnd_file, gnd_file_list_refresh, bin_file):
        self.file_crc = 0
        self.flt_filename = flt_file
        self.gnd_filename = gnd_file
        self.gnd_file_list_refresh = gnd_file_list_refresh
        if self.file is not None:
            self.file.close()
            self.file = None
        if self.xfer_state not in ('IDLE', 'FINISH'):
            self.cancel_xfer()
        if bin_file:
            data_seg_len = int(Cfe.FILE_XFER_DATA_SEG_LEN/2) # Encoding doubles the data size
            self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'StartBinFotp', 'prm': {'DataSegLen': data_seg_len, 'DataSegOffset': 0, 'SrcFilename': ''.join(flt_file)}})

        else:
            self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'StartFotp', 'prm': {'DataSegLen': Cfe.FILE_XFER_DATA_SEG_LEN, 'DataSegOffset': 0, 'SrcFilename': ''.join(flt_file)}})

    def cancel_xfer(self):
        self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'CancelFotp', 'prm': {}})
        self.xfer_state = 'IDLE'
    
    def send_gnd_event(self, event_text, popup_event=False):
        if popup_event:
             self.gui_evt_queue.put(POPUP_EVENT)
        event_text = 'GND Event: ' + event_text
        self.gui_evt_queue.put(event_text)           


###############################################################################

class SendFile():
    """
    This is the ground component of the FSW File Input Transfer Protocol (FITP).
    """
    def __init__(self, gui_evt_queue: Queue, flt_cmd_queue: Queue):
        
        self.gui_evt_queue = gui_evt_queue
        self.flt_cmd_queue = flt_cmd_queue

        self.file_crc = 0
        self.bytes_read = 0

        self.data_seg_delay = 0.50 #TODO: Create a better flow control mechanism

    def start_bin_xfer(self, gnd_file, flt_file):
        """
        Send a binary file to the cFS as hex encoded text file. There's interest in 
        using NASA apps with Basecamp so binary file transfer is needed. Prior to this 
        I didn't use the python encode/decode because I already had my FSW PktUtil_
        functions and I didn't feel like researching a standards-based solution.
        The data_seg_len represents the binary data length and not the encoded length.
        """
        max_bin_data_seg_len = int(Cfe.FILE_XFER_DATA_SEG_LEN/2) # Encoding doubles the data size
        self.file_crc = 0
        self.bytes_read = 0
        data_seg_id = Cfe.FILE_XFER_DATA_SEG_ID_START
        file_len = os.stat(gnd_file).st_size
        
        self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'StartBinFitp', 'prm': {'DestFilename': flt_file}})
        
        with open(gnd_file,'rb') as f: 
            while True:
                bin_data_segment = f.read(max_bin_data_seg_len)
                if not bin_data_segment: # Null indicates EOF
                    break
                self.file_crc = zlib.crc32(bin_data_segment, self.file_crc)               
                hex_data_segment = bin_hex_encode(bin_data_segment)
                if len(hex_data_segment) != int(len(bin_data_segment)*2):
                    status_str = f'Error encoding binary data segment. hex_len: {len(hex_data_segment)}, bin_len: {len(bin_data_segment)}'
                    self.send_gnd_event(status_str, popup_event=True)
                #TODO: Properly handle error case
                self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'SendFitpData', 'prm': {'Id': data_seg_id, 'Len': len(bin_data_segment), 'Data': hex_data_segment}})
                data_seg_id += 1
                time.sleep(self.data_seg_delay)
            self.file_crc = self.file_crc & 0xFFFFFFFF  # Ensure unsigned 32-bit result
            self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'FinishFitp', 'prm': {'FileLen': file_len, 'FileCrc': self.file_crc, 'LastDataSegmentId': data_seg_id-1}})

    def start_txt_xfer(self, gnd_file, flt_file):
        """
        Send a file to the cFS. This is a prototype 
        TODO - Generalize to binary once the EDS binary block updates are implemented
        """
        self.file_crc = 0
        self.bytes_read = 0
        data_seg_id = Cfe.FILE_XFER_DATA_SEG_ID_START
        file_len = os.stat(gnd_file).st_size
        
        self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'StartFitp', 'prm': {'DestFilename': flt_file}})        
        self.send_gnd_event(f'Sent FILE_XFER StartFitp (DestFilename: {flt_file})')

        # https://stackoverflow.com/questions/52722787/problem-sending-binary-files-via-sockets-python
        # send file size as big endian 64 bit value (8 bytes)
        # self.sock.sendall(os.stat(gnd_file).st_size.tobytes(8,'big'))
        with open(gnd_file,'r') as f: 
            while True:
                data_segment = f.read(Cfe.FILE_XFER_DATA_SEG_LEN) #bytearray(f.read(64))
                if not data_segment: # Null indicates EOF
                    break
                print(f'BC: Sending FITP data segment {data_seg_id}')
                self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'SendFitpData', 'prm': {'Id': data_seg_id, 'Len': len(data_segment), 'Data': data_segment}})        
                self.file_crc = zlib.crc32(data_segment.encode("utf-8"), self.file_crc)
                data_seg_id += 1
                time.sleep(self.data_seg_delay)
            self.file_crc = self.file_crc & 0xFFFFFFFF  # Ensure unsigned 32-bit result
            print(f'BC: Sending FITP finish file len {file_len}')
            self.flt_cmd_queue.put({ 'app': 'FILE_XFER', 'cmd': 'FinishFitp', 'prm': {'FileLen': file_len, 'FileCrc': self.file_crc, 'LastDataSegmentId': data_seg_id-1}})        
            self.send_gnd_event(f'Sent FILE_XFER FinishFitp (FileLen: {file_len}, FileCrc: {self.file_crc}, LastDataSegmentId: {data_seg_id-1})')
    
    def send_gnd_event(self, event_text, popup_event=False):
        if popup_event:
             self.gui_evt_queue.put(POPUP_EVENT)
        event_text = 'GND Event: ' + event_text
        self.gui_evt_queue.put(event_text)  

    def tlm_callback(self, tlm_msg: TelemetryMessage):
        """
        """
        pass


###############################################################################

class FileBrowserTelemetryMonitor(TelemetryObserver):
    """
    callback_functions
       [app_name] : {packet: [item list]} 
    
    """

    def __init__(self, tlm_server: TelemetrySocketServer, tlm_monitors, gui_evt_queue: Queue, filemgr_callback, send_file_callback, recv_file_callback): 
        super().__init__(tlm_server)

        self.tlm_monitors  = tlm_monitors
        self.gui_evt_queue = gui_evt_queue
        self.filemgr_callback = filemgr_callback
        self.send_file_callback = send_file_callback
        self.recv_file_callback = recv_file_callback
        
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
                event_text = "FSW Event at %s: %s, %s - %s" % \
                             (str(tlm_msg.sec_hdr().Seconds), pkt_id.AppName, Cfe.EVS_TYPE_STR[int(pkt_id.EventType)], payload.Message)
                self.gui_evt_queue.put(event_text)
                
        elif tlm_msg.app_name == 'FILE_XFER':
            ##self.gui_evt_queue.put(f'BC: FILE_XFER Msg: {tlm_msg.msg_name}') #TODO
            if 'FOTP' in tlm_msg.msg_name:
                self.recv_file_callback(tlm_msg)
              
                
###############################################################################

class FileBrowser(CmdTlmProcess):
    """
    Provide a user interface for managing ground and flight directories and
    files. It also supports transferring files between the flight and ground.
    """
    def __init__(self, mission_name, gnd_ip_addr, router_ctrl_port, browser_cmd_port, browser_tlm_port, browser_tlm_timeout):
        super().__init__(mission_name, gnd_ip_addr, router_ctrl_port, browser_cmd_port, browser_tlm_port, browser_tlm_timeout)

        self.bc_path_mgr = BcPathMgr()
        self.default_gnd_path = str(self.bc_path_mgr.get(BcPath.FLT_FILE_SERVER))
        self.default_flt_path = BcPathConst.CFS_STARTUP_PATH
        self.event_history = ""
        self.init_cycle = True
        self.popup_event = False

        self.event_queue   = queue.Queue()
        self.flt_cmd_queue = queue.Queue()  
        self.gnd_dir_queue = queue.Queue()  
        self.flt_dir_queue = queue.Queue()  
        
          
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
        self.gnd_file_menu = ['_', ['Refresh', '---', 'Send Text to Flight', 'Send Binary to Flight', '---', 'Edit File', 'Edit cFS Table', 'Rename File', 'Delete File']] #TODO - Decide on dir support 
        self.gnd_col = [
            [sg.Text('Ground', font=col_title_font)],
            [sg.Text('Folder'), sg.In(self.default_gnd_path, size=(25,1), enable_events=True ,key='-GND_FOLDER-'), sg.FolderBrowse(initial_folder=self.default_gnd_path)],
            [sg.Listbox(values=[], font=list_font, enable_events=True, size=(50,20), key='-GND_FILE_LIST-', right_click_menu=self.gnd_file_menu)]]
        
        # Duplicate ground names have a trailing space to differentiate them. A little kludgy but it works
        self.flt_file_menu = ['_', [ 'Refresh ', '---', 'List Dir', 'Send Text to Ground', 'Send Binary to Ground', 'Cancel Send', '---',  'Create Dir', 'Delete Dir', '---', 'Rename File ', 'Delete File ']] 
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
        
        self.gnd_dir   = GroundDir(self.default_gnd_path, self.gnd_dir_queue, self.event_queue)
        self.flt_dir   = FlightDir(self.default_flt_path, self.flt_dir_queue, self.event_queue, self.flt_cmd_queue)
        self.send_file = SendFile(self.event_queue, self.flt_cmd_queue)
        self.recv_file = RecvFile(self.event_queue, self.flt_cmd_queue)
        
        self.tlm_monitors = {'CFE_ES': {'HK_TLM': ['Seconds']}, 'FILE_MGR': {'DIR_LIST_TLM': ['Seconds']}}        
        self.tlm_monitor = FileBrowserTelemetryMonitor(self.tlm_server, self.tlm_monitors, self.event_queue, self.flt_dir.filemgr_dir_list_callback, self.send_file.tlm_callback, self.recv_file.tlm_callback)
        self.tlm_server.execute()

        while True:

            self.event, self.values = self.window.read(timeout=50)

            if self.init_cycle:
                self.init_cycle = False
                self.gnd_dir.create_file_list(self.default_gnd_path)
                self.flt_dir.create_file_list(self.default_flt_path)

            if not self.event_queue.empty():
                new_event_text = self.event_queue.get_nowait()
                if new_event_text == POPUP_EVENT:
                    self.popup_event = True
                else:
                    self.display_event(new_event_text)
                    if self.popup_event:
                        sg.popup(new_event_text, title='File Transfer Error', grab_anywhere=True, modal=False)
                        self.popup_event = False

            if not self.flt_cmd_queue.empty():
                new_cmd = self.flt_cmd_queue.get_nowait()
                self.send_cfs_cmd(new_cmd['app'], new_cmd['cmd'], new_cmd['prm'])

            if not self.gnd_dir_queue.empty():
                gnd_file_list = self.gnd_dir_queue.get_nowait()
                self.window['-GND_FILE_LIST-'].update(gnd_file_list)

            if not self.flt_dir_queue.empty():
                flt_file_list = self.flt_dir_queue.get_nowait()
                self.window['-FLT_FILE_LIST-'].update(flt_file_list)
                   
            ### Admin ###

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                break

            elif self.event == '-CLEAR_EVENTS-':
                self.event_history = ""
                self.display_event("Cleared event display")


            ### File Transfer ###

            elif self.event in ('Send Text to Flight', 'Send Binary to Flight'):
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-GND_FILE_LIST-'][0])
                    gnd_file = self.gnd_dir.path_filename(filename)
                    flt_file = self.flt_dir.path_filename(filename)
                    if self.event == 'Send Text to Flight':
                        self.send_file.start_txt_xfer(gnd_file, flt_file)
                    else:
                        self.send_file.start_bin_xfer(gnd_file, flt_file)
                    self.flt_dir.create_file_list()
                else:
                    sg.popup("Please select/highlight a file to be transferred to the cFS", title='Send File to Flight', grab_anywhere=True, modal=False)
            
            elif self.event in ('Send Text to Ground', 'Send Binary to Ground'):
                if len(self.values['-FLT_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-FLT_FILE_LIST-'][0])
                    flt_file = self.flt_dir.path_filename(filename)
                    gnd_file = self.gnd_dir.path_filename(filename)
                    print('>>>>flt_file: %s, gnd_file: %s' % (flt_file, gnd_file))
                    if self.event == 'Send Text to Ground':
                        self.recv_file.start_xfer(flt_file, gnd_file, self.gnd_dir.create_file_list, False)
                    else:
                        self.recv_file.start_xfer(flt_file, gnd_file, self.gnd_dir.create_file_list, True)                    
                    #TODO - Trigger ground file list display refresh
                else:
                    sg.popup("Please select/highlight a file to be transferred to the ground", title='Send File to FLight', grab_anywhere=True, modal=False)
                               
            elif self.event == 'Cancel Send':
                self.recv_file.cancel_xfer()

            ### Ground ###
                
            elif self.event == '-GND_FOLDER-':                         
                self.gnd_dir.create_file_list(self.values['-GND_FOLDER-'])

            elif self.event == 'Refresh':                         
                self.gnd_dir.create_file_list()

            elif self.event == 'Edit File':
                """
                Only loads initial file if it is of a particular type. Does allow editor to be 
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

            elif self.event == 'Edit cFS Table':
                """
                Only loads initial file if it is of a particular type. Does allow editor to be 
                launched if no file selected. 
                """
                #TODO - Use ini file config
                app_path = os.getcwd()
                if 'cfsinterface' not in app_path:
                    app_path = os.path.join(app_path, 'cfsinterface')
                filename = ''
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    filename = self.get_filename(self.values['-GND_FILE_LIST-'][0])
                    if filename.endswith((".tbl", ".bin")):
                        filename = self.gnd_dir.path_filename(filename)
                self.table_manager = sg.execute_py_file("tblmanager.py", parms=filename, cwd=app_path)

            elif self.event == 'Delete File':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                   filename = self.get_filename(self.values['-GND_FILE_LIST-'][0])
                   self.gnd_dir.delete_file(filename)

            elif self.event == 'Rename File':
                if len(self.values['-GND_FILE_LIST-']) > 0:
                    dst_file = sg.popup_get_text(title='Rename ' + self.values['-GND_FILE_LIST-'][0], message='Please enter the new filename (not full path)')
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
                dir_name = sg.popup_get_text(title='Create Directory', message='Please enter new directory name (not full path)')
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
                    dst_file = sg.popup_get_text(title='Rename '+filename, message='Please enter the new filename (not full path)')
                    if dst_file is not None:
                        self.flt_dir.rename_file(filename, dst_file)
                        self.window['-FLT_FOLDER-'].update(self.flt_dir.path)
        
        self.shutdown()


    def execute(self):
        self.gui()
    
        
    def shutdown(self):

        self.window.close()       
        self.tlm_server.shutdown()    


###############################################################################

if __name__ == '__main__':
    """
    This is run as a separate Basecamp process with a command line arguments:

    To run this standalone do the following:
       1. In basecamp.py ~ line 1326 comment out 
          self.file_browser = sg.execute_py_file("filebrowser.py",...
       2. Start basecamp
       3. Launch File Browser to add the cmd/tlm networks routes. Note browser
          won't launch because it's commented out 
       4. Start cFS
       5. Start FileBrowser in a separate terminal
    """
    #print(f"Name of the script      : {sys.argv[0]}")
    #print(f"Arguments of the script : {sys.argv[1]}")

    bc_path_mgr = BcPathMgr()
    
    config = configparser.ConfigParser()
    config.read(bc_path_mgr.get_basecamp_ini_file())

    #tlm_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #tlm_socket.sendto("127.0.0.1,1234".encode(),("127.0.0.1",8888))

    cfs_ip_addr = config.get('NETWORK','CFS_IP_ADDR')
    router_ctrl_port = config.getint('NETWORK','CMD_TLM_ROUTER_CTRL_PORT')
    browser_cmd_port = config.getint('NETWORK','FILE_BROWSER_CMD_PORT')
    browser_tlm_port = config.getint('NETWORK','FILE_BROWSER_TLM_PORT')
    mission_name     = config.get('CFS_TARGET','MISSION_NAME')
    
    file_browser = FileBrowser(mission_name, cfs_ip_addr, router_ctrl_port, browser_cmd_port, browser_tlm_port, 2.0)
    file_browser.execute()
