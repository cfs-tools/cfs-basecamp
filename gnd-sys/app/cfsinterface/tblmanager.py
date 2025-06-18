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
      Display, edit and save cFS binary tables
      
    Notes:
      1. The telemetry is displayed in a text box with each row containing
         one data point. The data point labels are extracted from the EDS
         message definition. The screen layout can't be altered.
                
"""

import sys
import time
import os
import socket
import configparser
import io

if __name__ == '__main__':
    sys.path.append('..')
    from cfeconstants  import Cfe
    from cfefile       import CfeFile
else:
    from .cfeconstants  import Cfe
    from .cfefile       import CfeFile
from tools import crc_32c, compress_abs_path, TextEditor, PySimpleGUI_License
import PySimpleGUI as sg

WINDOW_TITLE  = 'Table Manager'
TBL_FILE_EXT  = 'tbl'

###############################################################################

class HelpText():
    """
    """
    def __init__(self):
  
        self.text = \
           ("Table Manager allows users to load a binary cFS table from a\n"
           "file, modify the data values and save changes to a file. Tables\n"
           "must be defined in an app's EDS spec using Basecamp's psuedo\n"
           "telemtry table file EDS conventions. This convention is a temporary\n"
           "work around until NASA supports EDS table file definitions as\n"
           "part of the EDS 'required interface' definition.")
            
    def display(self):
            
        sg.popup(self.text, line_width=85, font=('Courier',12), title='Command Sequencer Help', grab_anywhere=True)


###############################################################################

class TblManager():
    """
    """

    def __init__(self, mission, target, flt_server_path): 
        
        self.flt_server_path = flt_server_path
        self.tbl_file = CfeFile(mission, target)
        self.tbl_data_modified = False

        self.top_row = ['Parameter', 'Value']
        self.help_text = HelpText()
        self.window_title = WINDOW_TITLE

    def create_window(self):
        """      
        """
        tbl_data_array = [['' for column in range(2)] for row in range(5)]
 
        window_width = 100
        col_width  = int(window_width/2-3)
        col_height = 10
        menu_layout = [['File',['&Open...','&Save','Save As...','---','Help','Exit']]]

        tbl_widget = sg.Table(values=tbl_data_array, headings=self.top_row,
                        auto_size_columns=True,
                        display_row_numbers=False,
                        justification='left', key='-TABLE-',
                        selected_row_colors='red on yellow',
                        enable_events=True,
                        expand_x=True,
                        expand_y=True,
                        enable_click_events=True)
                        
        hdr_label_font   = ('Arial bold',12)
        hdr_content_font = ('Arial',12)

        self.layout = [
            [sg.Menu(menu_layout)],
            [sg.Text('Click on a data parameter row to edit its contents.', font=hdr_label_font)],
            [sg.Text('Parameter: ', font=hdr_label_font, size=(10,1)), sg.Text('', font=hdr_content_font, size=(75,1), border_width=1, justification='left', key='-PARAMETER-')],
            [sg.Text('Value: ', font=hdr_label_font, size=(10,1)), sg.Input('', font=hdr_content_font, size=(25,1), border_width=1, justification='right', key='-DATA-'), sg.Button('Update', enable_events=True, button_color=('SpringGreen4'), key='-UPDATE-', pad=((5,5),(12,12))),], #, relief=sg.RELIEF_RAISED
            [[tbl_widget]]]
             
        window = sg.Window(self.window_title, self.layout, resizable=True)        
        
        return window


    def reset_tbl_data_modified(self):
        self.window_title.replace('*','')
        self.window.set_title(self.window_title)
        self.tbl_data_modified = True

        
    def execute(self):
        """
        The current value observer must be created after the GUI window is created and the
        first window read is performed 
        """

        self.window = self.create_window()

        while True:

            event, values = self.window.read()

            print(f'Event: {event}')
            if event in (sg.WIN_CLOSED, 'Close', 'Exit') or event is None:       
                if self.tbl_data_modified:
                    save_file = sg.popup_yes_no('The data has been modified.\nDo you want to save it?', title='Save Modified Data', grab_anywhere=True, modal=False)
                    if save_file == 'Yes':
                        path_filename = sg.popup_get_file('Save File', save_as=True, no_window=True)
                        self.tbl_file.write_file(path_filename)
                break

            elif isinstance(event, tuple) and event[0] == '-TABLE-' and event[1] == '+CLICKED+':
                row,col = event[2]
                print(f'Table[{row},{col}]: {self.tbl_file.tbl_data_array[row][0]}: {self.tbl_file.tbl_data_array[row][1]}')
                self.window['-PARAMETER-'].update(self.tbl_file.tbl_data_array[row][0])
                self.window['-DATA-'].update(self.tbl_file.tbl_data_array[row][1])

            elif event == '-UPDATE-':
                self.window['-TABLE-'].update(values=self.tbl_file.update_data_array(row,1,values['-DATA-']))
                # Add data modified indicator on first update
                if not self.tbl_data_modified:
                    self.window_title = '*' + self.window_title
                    self.window.set_title(self.window_title)
                self.tbl_data_modified = True
                
            elif event == 'Open...':
                tbl_file = sg.popup_get_file('', title='Table File', no_window=True, default_path=self.flt_server_path, initial_folder=self.flt_server_path, file_types=(("Table Files", "*.tbl"),), default_extension=TBL_FILE_EXT) # , history=True)
                tbl_data_array = self.tbl_file.read(tbl_file,'FILE_MGR/Application/FILE_SYS_TBL_FILE') # TODO: Add topic ID support
                if len(tbl_data_array) > 6:  # TODO: Pick number that ensures at least the file headers exist
                    # TODO: Remove backdoor test 
                    if tbl_file is None:
                        tbl_file = 'FILE_MGR_SYS_TBL_TLM'
                    self.window_title = f'{WINDOW_TITLE}: {os.path.basename(tbl_file)}'
                    self.window.set_title(self.window_title)
                    self.window['-TABLE-'].update(values=tbl_data_array)
                    self.tbl_data_modified = False
            
            elif event == 'Save':
                self.tbl_file.write()
                self.reset_tbl_data_modified()

            elif event == 'Save As...':
                path_filename = sg.popup_get_file('Save File', save_as=True, no_window=True)
                self.tbl_file.write_file(path_filename)
                self.reset_tbl_data_modified()

            elif event == 'Help':
                self.tbl_file.create_byte_data()
                #self.help_text.display()
                
        self.window.close()


        
###############################################################################

if __name__ == '__main__':
    
    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    MISSION         = config.get('CFS_TARGET', 'MISSION_EDS_NAME')
    CFS_TARGET      = config.get('CFS_TARGET', 'CPU_EDS_NAME')
    FLT_SERVER_PATH = config.get('PATHS','FLT_SERVER_PATH')
    FLT_SERVER_PATH = compress_abs_path(os.path.join(os.getcwd(), '..', FLT_SERVER_PATH))
    
    tbl_manager = TblManager(MISSION, CFS_TARGET, FLT_SERVER_PATH)
    tbl_manager.execute() 
    
