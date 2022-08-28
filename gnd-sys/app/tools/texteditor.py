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
        Provide a simple text editor to assist users with their app development
        workflows. This is not intended to replace a full-featured editor.
  
    TODO - Resolve all TODO dialogs
    TODO - Add dirty flag and prompt user to save before exit without save
          
"""
import sys
import time
import os
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

import PySimpleGUI as sg


###############################################################################

class EditorConfig():
    """
    """
    def __init__(self):
  
        self.config = {
            'theme':   'BluePurple',
            'themes':  sg.list_of_look_and_feel_values(),
            'font':    ('Consolas', 12),
            'tabsize': 4 
            }
        
    def get(self, type):
        config = None
        if type in self.config:
            config = self.config[type]
        return config


###############################################################################

class HelpText():
    """
    """
    def __init__(self):
  
        self.text = {

            'NEW_FILE': 
                ("This is a simple text file editor intended to help with quick edits to assist users\n"
                 "with their app development workflows. The 'Execute' menu provides quick access to\n"
                 "common activities like building the cFS. The following file types are recognized by\n"
                 "the editor and the lower GUI pane provides file specific guidance.\n\n"
                 "  '*.py'               - User test scripts\n"
                 "  '*.json'             - cFS application initialization and table files\n"
                 "  'targets.cmake'      - Define cFS build target apps and files\n"
                 "  'cfe_es_startup.scr' - Define apps and libraries loaded during cFS initialization\n\n"),

            'CMAKE':
                ("targets.cmake is the top-level cmake cFS target configration file. The following\n"
                 "variables are used to add an app to a target:\n\n"
                 "  cpu1_APPLIST  - List of apps to be included in CPU1 target\n"
                 "  cpu1_FILELIST - List of files to copy from basecamp_defs to the '/cf' directory\n"),
                       
            'PYTHON':
                ("User python scripts are executed within the context of the ScriptRunner class that\n"
                 "provides a simple command and telemetry API. ScriptRunner is not designed to provide\n"
                 "a comprehensive test and operational environment. See scriptrunner.py for a \n"
                 "description of the API and demo_script.py for an example script\n"),

            'SCR':
                ("cfe_es_startup.scr defines which cFS libraries and apps are loaded during startup. The\n"
                "file's comment describe how to format/defince script entries. The following libraries\n"
                "and apps must be loaded:\n\n"
                "cfe_assert  - Provides unit test runtime framework, only required unit tests\n"
                "osk_c_fw    - OpenSatKit app framework. Required by all OSK apps and mustbe loaded priori to apps\n"
                "ci_lab      - Command Ingest app receives commands from basecamp.py and transmits them on the software bus\n"
                "to_lab      - Telemetry Output app reads messages from software bus and transmits them to basecamp.py\n"
                "sch_lab     - Scheduler app periodically sends software bus message to trigger app functions\n"
                "file_mgr    - File Manager app provides basic directory and file management services\n"
                "file_xfer   - File Transfer app trasnfers files between basecamp.py and a cFS target\n"),       

            'JSON':
                ("JSON files are used by apps using the OpenSatKit app framework to define app initialization\n"
                "configuration parameters. They are also used for app 'tables' that are used to define parameters\n"
                "that can be changed during runtime using table loads. JSON tables can be written to a file,\n"
                "transferred to the ground using FILE_XFER, and displayed in a text editor"),
            
            'OTHER':  ("No guidance for this file type")
                
            }
            
    def display(self, filename):
        file_type = 'NEW_FILE'
        if filename not in (None, ''):
            file_type = 'OTHER'
            file_ext = filename.split('.')[-1]
            if file_ext == 'cmake':
                file_type = 'CMAKE'
            elif file_ext == 'scr':
                file_type = 'SCR' 
            elif file_ext == 'json':
                file_type = 'JSON'
            elif file_ext == 'py':
                file_type = 'PYTHON'
            
        sg.popup(self.text[file_type], line_width=85, font=('Courier',12), title='File Guidance', grab_anywhere=True)


###############################################################################

class TextEditor():
    """
    Provide a user interface for managing ground and flight directories and
    files. It also supports transferring files between the flight and ground.
    """
    
    NEW_FILE_STR = '-- New File --'
    
    def __init__(self, filename=None, build_cfs_callback=None, run_script_callback=None):        
      
        self.build_cfs_callback  = build_cfs_callback
        self.run_script_callback = run_script_callback
        
        self.text_modified = False
        self.filename = None
        if filename not in (None, ''):
           if os.path.isfile(filename):
               self.filename = filename
           
        self.config = EditorConfig()
        self.help_text = HelpText()
    
    def open_file(self, filename, window):
        
        if filename not in (None,''):
            if os.path.isfile(filename):
                self.filename = filename
                with open(self.filename,'r') as f:
                    file_text = f.read()
                window['-FILE_TEXT-'].update(value=file_text)
                window['-FILE_TITLE-'].update(value=self.filename)
        self.text_modified = False
    
    def save_file(self, filename, window):
        """
        This function protects against multiple filename empty types. None
        and a string length of 0 are most common, but this could be called 
        with the return value after a cancelled save so filename could be
        a zero length tuple.
        """
        updated = False
        if filename is not None:
            if len(self.filename) > 0:
                with open(filename,'w') as f:
                    f.write(self.values['-FILE_TEXT-'])
                window['-FILE_TITLE-'].update(value=filename)
                self.filename = filename
                updated = True
                               
        if not updated:
            window['-FILE_TITLE-'].update(value = self.NEW_FILE_STR)
        self.text_modified = False

    def save_filename(self, window):
        if self.filename in (None,''):
            try: # Some platforms may raise exceptions on cancel
                filename = sg.popup_get_file('Save File', save_as=True, no_window=True)
                self.save_file(filename, window)
            except:
                pass
        else:
            self.save_file(self.filename, window)

    def text_left_click(self):
        print('text_left_click')
        
    def create_window(self):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        This editor is intentionally very simple. I orginally had the guidance as a second window
        pane but this wastes screen space and is annoying when you don't need the guidance.
        """
        window_width = 100
        menu_layout = [
                ['File',['New','Open','&Save','Save As','---','Exit']],
                ['Edit',['Select &All','Cut','&Copy','Paste','Undo','---','&Find...','Replace...']],
                ['Execute',['Build cFS', 'Run Script']],
            ]

        self.file_text = sg.Multiline(default_text='', font=self.config.get('font'), enable_events=True, key='-FILE_TEXT-', size=(window_width,30))
        
        window_layout = [
            [sg.Menu(menu_layout)],
            [[
              sg.Button('Help', enable_events=True, key='-FILE_HELP-', pad=((5,5),(12,12))),
              sg.Text(self.NEW_FILE_STR, key='-FILE_TITLE-', font=self.config.get('font'), size=(window_width-10,1))
            ]],
            [sg.Column([[self.file_text]])]]


        window = sg.Window('Text File Editor', window_layout, resizable=True, margins=(0,0), return_keyboard_events=True, finalize=True)
        return window
       
                
    def gui(self):
    
        window = self.create_window()
        
        #TODO window.find_element['-FILE_TEXT-'].Widget.bind('<Button-1>', self.text_left_click()) #Copy') #
        #TODO self.file_text.Widget.bind('<Button-1>', self.text_left_click())
        
        if self.filename is not None:
            self.open_file(self.filename, window)
        
        prev_encoded_event = None
        
        while True:

            self.event, self.values = window.read() # (timeout=50) - Using a timeout causes the ctrl scheme below to crash but not sure why 
            
            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                if self.text_modified:
                   save_file = sg.popup_yes_no('The text has been modified.\nDo you want to save it?', title='TODO', grab_anywhere=True, modal=False)
                   if save_file == 'Yes':
                       self.save_filename(window)
                break

            # The goal is to capture control-key events in a portable way so I avoided the tkinter event
            # binding method. However, I've only tested this on tkinter so it may not be portable.
            # When a control-key sequence is pressed two events are generated. key followed by a delayed
            # Control_L:37. This logic saves the key and creates a new encoded string.
            # Seems like a hack, but it works!
            
            encoded_event = str(str(self.event).encode('utf-8'))  
                        
            if encoded_event in ("b'Control_L:37'"):
                if prev_encoded_event is not None:
                    encoded_event = encoded_event.split(':')[0] + ':' + prev_encoded_event.split(':')[1]
                    prev_encoded_event = None
            
            if encoded_event in ("b'-FILE_HELP-'"):
                self.help_text.display(self.filename)
           
            elif encoded_event in ("b'-FILE_TEXT-'",):
                if not self.text_modified: 
                    self.text_modified = True
                    new_title = '*' + window['-FILE_TITLE-'].get()
                    window['-FILE_TITLE-'].update(value=new_title)
                
            ### File Menu ###

            elif encoded_event in ("b'New'",):
                self.filename = None
                window['-FILE_TEXT-'].update(value = '')
                window['-FILE_TITLE-'].update(value = self.NEW_FILE_STR)
                
            elif encoded_event in ("b'Open'",):
                try: # Some platforms may raise exceptions
                    filename = sg.popup_get_file('File Name:', title='Open', no_window=True)
                    self.open_file(filename, window)
                except:
                    pass
              
            elif encoded_event in ("b'Save'","b'Control_L:39'"): # s=39
                self.save_filename(window)
                
            elif encoded_event in (b'Save As',):
                try: # Some platforms may raise exceptions on cancel
                    filename = sg.popup_get_file('Save File', save_as=True, no_window=True)
                    self.save_file(filename, window)
                except:
                    pass                
            
            ### Edit Menu ###
                
            elif encoded_event in ("b'Select All'", "b'Control_L:38'"): # a=38
                sg.popup("<Select All> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif encoded_event in ("b'Cut'", "b'Control_L:53'"): # x=53
                sg.popup("<Cut> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif encoded_event in ("b'Copy'", "b'Control_L:54'"): # c=54
                selection = window['-FILE_TEXT-'].Widget.selection_get()
                sg.clipboard_set(selection)
                
            elif encoded_event in ("b'Paste'", "b'Control_L:55'"): # v=55
                sg.popup("<Paste> not implemented. Clipboard contains '%s' "%sg.clipboard_get(), title='TODO', grab_anywhere=True, modal=False)

            elif encoded_event in ("b'Undo'", "b'Control_L:52'"): # z=52
                sg.popup("<Undo> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif encoded_event in ("b'Find...'", "b'Control_L:41'"): # z=42
                sg.popup("<Find...> not implemented", title='TODO', grab_anywhere=True, modal=False)

            elif encoded_event in ("b'Replace...'",):
                sg.popup("<Replace...> not implemented", title='TODO', grab_anywhere=True, modal=False)


            ### Execute ###

            elif encoded_event in  ("b'Build cFS'",):
                if self.build_cfs_callback is None:
                    sg.popup("<Build cFS> is not supported in this context", title='Information', grab_anywhere=True, modal=False)
                else:
                    self.self.build_cfs_callback
                    
            elif encoded_event in  ("b'Run Script'",):
                if self.run_script_callback is None:
                    sg.popup("<Run Script> is not supported in this context", title='Information', grab_anywhere=True, modal=False)
                else:
                    self.run_script_callback(self.values['-FILE_TEXT-'])

            prev_encoded_event = encoded_event
            
        window.close()
        
    def execute(self):
        self.gui()
    

###############################################################################

if __name__ == '__main__':
    """
    sys.argv[0] - Name of script
    sys.argv[1] - If provided is the filename to be edited
    """
    #Raspberry Pi Python gave <ftype> error on these lines
    #print(f"Name of the script      : {sys.argv[0]=}")
    #print(f"Arguments of the script : {sys.argv[1:]=}")

    filename = None
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        #print ('filename = ' + filename)
        
    text_editor = TextEditor(filename)
    text_editor.execute()
    



