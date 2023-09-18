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
        Provide classes to manage an app's code tutorials and lessons. The
        text editor for a implementing a coding lesson is veru simple and
        is not intended to replace a full-featured editor.

    Notes:
      1. JSON key constants should all be used within the Json class so if any key
         changes only the Json class will change.
      TODO - Crashes if try to save a modified file when exit selected
"""

import sys
import time
import os
import json
import configparser
import fitz
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from jsonfile      import JsonFile
    from pdfviewer     import PdfViewer
    from utils         import compress_abs_path
else:
    from .jsonfile      import JsonFile
    from .pdfviewer     import PdfViewer
    from .utils         import compress_abs_path

import PySimpleGUI as sg

TUTORIAL_JSON_FILE = 'tutorial.json'
TUTORIAL_DIR       = 'tutorial'
LESSON_JSON_FILE   = 'lesson.json'
LESSON_DIR         = 'lesson'
DOCS_DIR           = 'docs'


###############################################################################

class CodeTutorialJson(JsonFile):
    """
    Manage tutor and lesson JSON files. Keep tag names and formats consistent.
    The tutor and lesson JSON files have nearly identical keys and since this is
    not a generic utility this one class takes care of both file types.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        
    def document(self):
        return self.json['document']

    
###############################################################################

class CodeTutorial():
    """
    Manage the display for a tutorial. A tutorial folder contains a
    tutorial.json that describes the tutorial and a lesson folder that 
    contains numbered lesson folders. Each lesson folder contains a 
    lesson.json file.  
    """
    def __init__(self, tutorial_path):

        self.path = tutorial_path
        self.json = CodeTutorialJson(os.path.join(tutorial_path, TUTORIAL_JSON_FILE))
        
        self.lesson_path = os.path.join(tutorial_path, LESSON_DIR)
        self.lesson_list = [int(l) for l in os.listdir(self.lesson_path) if l.isnumeric()]
        self.lesson_list.sort()

        self.lesson_objs = {}
        self.display = True
        self.reset   = False


    def create_lesson_objs(self):
        self.lesson_objs = {}
        for l in self.lesson_list:
            lesson_num_path = os.path.join(self.lesson_path, str(l))
            lesson_pathname = os.path.join(lesson_num_path, LESSON_JSON_FILE)
            if os.path.exists(lesson_pathname):
                self.lesson_objs[l] = CodeLesson(self.json.title(), l, lesson_num_path)            
            else:
                print(f'{LESSON_JSON_FILE} not found for lesson {l}')
                
    def create_window(self):
        """
        Create the main window. Non-class variables are used so it can be refreshed,
        PySimpleGui layouts can't be shared.
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        
        objective_text = ""
        for objective_line in self.json.objective():
            objective_text += objective_line

        resume_lesson = 1
        for lesson in self.lesson_objs.values():
            if lesson.complete:
                resume_lesson += 1
            else:
                break
                    
        lesson_layout = []
        for lesson in self.lesson_objs.values():
            title          = f'{lesson.number}-{lesson.title}'
            complete_state = 'Yes' if lesson.complete else 'No'
            radio_state    = True if lesson.number == resume_lesson else False
            lesson_layout.append([sg.Radio(title, 'LESSONS', default=radio_state, font=hdr_value_font, size=(30,0), key=f'-LESSON{lesson.number}-'), sg.Text(complete_state, key=f'-COMPLETE{lesson.number}-')])
        
        # Layouts can't be reused/shared so if someone does a tutorial reset it causes issues if layout is a class variable
        layout = [
                  [sg.Text('Objectives', font=hdr_label_font), sg.Button('Document', pad=(2,0)) ],
                  [sg.MLine(default_text=objective_text, font = hdr_value_font, size=(40, 4))],
                  # Lesson size less than lesson layout so complete status will appear centered 
                  [sg.Text('Lesson', font=hdr_label_font, size=(30,0)),sg.Text('Complete', font=hdr_label_font, size=(10,0))],  
                  lesson_layout, 
                  [sg.Button('Start', button_color=('SpringGreen4'), pad=(2,0)), sg.Button('Reset', pad=(2,0)), sg.Button('Exit', pad=(2,0))]
                 ]

        window = sg.Window(self.json.title(), layout, modal=True)
        return window
        
        
    def selected_lesson(self):
        """
        Return the selected lesson.
        """
        lesson_obj = None
        for lesson in self.lesson_objs:
            if self.values[f'-LESSON{lesson}-'] == True:
               lesson_obj = self.lesson_objs[lesson]
               break
               
        return lesson_obj
    
    
    def gui(self):
        """
        Navigating through lessons is not strictly enforced.  The goal is to keep the user
        interface very simple so the algorithm to determine which lesson to resume is simplistic
        and it's up to the user whether they select lessons as completed.
        """
                
        while self.display:

            self.create_lesson_objs()
            window = self.create_window()

            while True: # Event Loop

                self.event, self.values = window.read(timeout=100)
                   
                if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:       
                    break
            
                elif self.event == 'Start':
                    lesson_obj = self.selected_lesson()
                    if lesson_obj is not None: 
                        if lesson_obj.execute():
                            window[f'-COMPLETE{lesson_obj.number}-'].update('Yes') 
                            self.reset = True
                            
                elif self.event == 'Objective':
                    lesson_obj = self.selected_lesson()
                    if lesson_obj is not None: 
                        objective = ""
                        for objective_line in lesson_obj.json.objective():
                            objective += objective_line
                        sg.popup(objective, title=lesson_obj.json.title())
                    else:
                        sg.popup("Please select an application template", title="Create Application", modal=False)
                
                elif self.event == 'Document':
                    pdf_filename = os.path.join(self.path, DOCS_DIR, self.json.document())
                    print(f'pdf_filename: {pdf_filename}')
                    pdf_viewer = PdfViewer(pdf_filename)
                    pdf_viewer.execute()

                elif self.event == 'Reset':
                    lesson_obj = self.selected_lesson()
                    if lesson_obj is not None: 
                        lesson_obj.reset()
                    self.reset = True
                    break
        
            self.json.update()
            window.close()
        
            if self.reset:
                self.reset = False
            else:
                self.display = False
        
    def execute(self):
        self.gui()


###############################################################################

class ManageCodeTutorials():
    """
    Discover what user app code tutorials exists. Each user app can have about
    tutorial directory. Create a 'database' of information about the tutorials
    based on each tutorial's JSON spec.
    User select tutorials based by title so self.tutorial_lookup provides a
    method to retrieve a tutorial given its title
    """
    def __init__(self, usr_app_path):

        self.tutorial_titles = []
        self.tutorial_lookup = {}  # [title]  => Tutorial
        
        usr_app_list = os.listdir(usr_app_path)
        usr_app_list.sort()
        for usr_app in usr_app_list:
            #todo: Tutorial constructor could raise exception if JSON doesn't exist or is malformed
            tutorial_dir = os.path.join(usr_app_path, usr_app, TUTORIAL_DIR)
            tutorial_json_file = os.path.join(tutorial_dir, TUTORIAL_JSON_FILE)
            if os.path.exists(tutorial_json_file):
                tutorial = CodeTutorial(tutorial_dir)
                self.tutorial_titles.append(tutorial.json.title())
                self.tutorial_lookup[tutorial.json.title()] = tutorial
        

###############################################################################

class CodeLessonJson(JsonFile):
    """
    Manage a lesson JSON file that has an array of files and each file has an
    array of exercises to complete the lesson.
    """
    def __init__(self, json_file):
        super().__init__(json_file)
        self.file_array = self.json['file']
        
    def current_file(self):
        return self.json['current-file']

    def file(self, index):
        return self.file_array[index]

    def lesson_complete(self):
        return self.json['complete']

    def reset_child(self):
        if 'current-file' in self.json:
           self.json['current-file'] = "Undefined"
           self.json['complete'] = False

    def set_complete(self, complete):
        self.json['complete'] = complete

    def set_lesson_file(self, file):
        self.json['current-file'] = file


###############################################################################

class CodeLesson():
    """
    Manage the display for a lesson. The lesson's JSON file is used to 
    determine the initial state. The execute() method allows a lesson to be
    restarted and override the JSON. The new lesson state is recorded in the
    JSON when the lesson is exited.
    """
    def __init__(self, tutorial, number, path):
        self.tutorial = tutorial
        self.number   = number
        self.path     = path
        self.load_json()

        self.file_idx = 0
        self.file     = self.json.file_array[self.file_idx]
        self.file_cnt = len(self.json.file_array)
        self.reset_exercise()

    def reset_exercise(self):
        self.exercise_idx = 0
        self.exercise     = self.file["exercise"][self.exercise_idx]
        self.exercise_cnt = len(self.file["exercise"])
         
    def exercise_id(self):
        return self.exercise['id']

    def exercise_instructions(self):
        return self.exercise['instructions']

    def lesson_file(self):
        return os.path.join(self.path, self.file['name'])

    def user_path_filename(self):
        return os.path.join(self.path, '../../..', self.file['path'], self.file['name'])

    def user_filename(self):
        return os.path.join(self.file['path'], self.file['name'])

    def increment_exercise(self):
        """
        Increment the exercise index and return the new exercise dictionary.
        """
        self.exercise_idx += 1
        if self.exercise_idx >= self.exercise_cnt:
            self.exercise_idx = 0
        self.exercise = self.file["exercise"][self.exercise_idx]
        return self.exercise

    def decrement_exercise(self):
        """
        Decrement the exercise index and return the new exercise dictionary.
        """
        self.exercise_idx -= 1
        if self.exercise_idx < 0:
            self.exercise_idx = self.exercise_cnt-1
        self.exercise = self.file["exercise"][self.exercise_idx]
        return self.exercise
        
    def increment_file(self):
        """
        Increment the file index and return the new file dictionary.
        """
        self.file_idx += 1
        if self.file_idx >= self.file_cnt:
            self.file_idx = 0
        self.file = self.json.file_array[self.file_idx]
        self.reset_exercise()
        return self.file

    def decrement_file(self):
        """
        Decrement the file index and return the new file dictionary.
        """
        self.file_idx -= 1
        if self.file_idx < 0:
            self.file_idx = self.file_cnt-1
        self.file = self.json.file_array[self.file_idx]
        self.reset_exercise()
        return self.file

    def load_json(self):
        self.json     = CodeLessonJson(os.path.join(self.path, LESSON_JSON_FILE))
        self.title    = self.json.title()
        self.complete = self.json.lesson_complete()

    def reset(self):
        self.json.reset()
        self.load_json()
        
    def execute(self):
        title = f'{self.tutorial} Lesson {self.number} - {self.title}'
        code_lesson_editor  = CodeLessonEditor(title, self);
        return code_lesson_editor.execute()
        

###############################################################################

class EditorConfig():
    """
    """
    def __init__(self):
  
        self.config = {
            'theme':   'BluePurple',
            'themes':  sg.list_of_look_and_feel_values(),
            'font':    ('Courier New', 12),
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
  
        self.text = \
           ("Code tutorials are organized into lessons that have a specific objective.\n"
           "A lesson may require one or more files to be edited. Each file may require one or\n"
           "more exercises to be completed in order to fulfill the lesson's objective.\n"
           "The bottom window pane contains the solution for an exercise. You may copy\n"
           "from the bottom window and paste into the top window which is the app's source\n"
           "file. You can also hand type solutions into the top window. The top window is\n"
           "the source file used when the application is built.\n\n"
           "Use Basecamp's main window's 'Build' button to rebuild the app.\n\n"
           "YOU MUST SAVE YOUR FILE BEFORE EXITING! A BUG PREVENTS THE FILE FROM BEING\n"
           "SAVED AFTER THE WINDOW CLOSE PROCESS HAS STARTED.")
            
    def display(self):
            
        sg.popup(self.text, line_width=85, font=('Courier',12), title='Code Tutorial Help', grab_anywhere=True)


###############################################################################

class CodeLessonEditor():
    """
    Provide a user interface for managing ground and flight directories and
    files. It also supports transferring files between the flight and ground.
    """
    
    NEW_FILE_STR = '-- New File --'
    
    def __init__(self, lesson_title, code_lesson, build_cfs_callback=None, run_script_callback=None):        
        """
        code_lesson - Instance of a CodeLesson object
        """
        self.title       = lesson_title
        self.code_lesson = code_lesson
        self.build_cfs_callback  = build_cfs_callback
        self.run_script_callback = run_script_callback
        
        self.lesson_completed  = False  # Was the lesson completed during this user session
        self.lesson_file_valid = False 
        self.lesson_filename   = None
        self.lesson_file_text  = ''   
        self.exercise_text     = ''   
        self.user_file         = None
        self.user_file_mod     = False        
        
        self.config    = EditorConfig()
        self.help_text = HelpText()
    
    
    def read_lesson_file(self):
        self.lesson_filename = self.code_lesson.lesson_file()   
        try:
            with open(self.lesson_filename,'r') as f:
                file_text = f.read()
            self.lesson_file_text = file_text.splitlines()
            self.lesson_file_valid = True
        except:
            self.lesson_file_valid = False
            sg.popup(f'Error opening lesson file {self.lesson_filename}', title='File Open Error')

    def update_file_displays(self, window):
        """
        If the file is updated then the lesson window must also be updated.
        """
        self.check_n_save_user_file(window)
        window['-USER_FILE-'].update(value=self.code_lesson.user_filename())
        self.read_lesson_file()
        self.write_lesson_window(window)
        self.open_user_file(window)
        
    def write_lesson_window(self, window):
        window_text = ''
        exercise_id = self.code_lesson.exercise_id()
        if exercise_id == '~FILE~':
            in_ex_block = True
        else:
            in_ex_block = False
        if self.lesson_file_valid:
            for line in self.lesson_file_text:
                if in_ex_block:
                    window_text += f'{line}\n'
                    if exercise_id in line:
                        in_ex_block = False
                else:
                    if exercise_id in line:
                        in_ex_block = True
                        window_text += f'{line}\n'
            self.exercise_text = window_text
        else:
            self.exercise_text = f'Lesson file {self.lesson_filename} has not been loaded'
        window['-LESSON_TEXT-'].update(value=self.exercise_text)
        window['-EXERCISE-'].update(value=self.code_lesson.exercise_id())
    
    def open_user_file(self, window):
        self.user_file = self.code_lesson.user_path_filename()
        try:
            with open(self.user_file,'r') as f:
                file_text = f.read()
            self.user_file_mod = False 
        except:
            file_text = f'File Not Found: {self.user_file}'
        window['-USER_TEXT-'].update(value=file_text)
        window['-USER_FILE-'].update(value=self.code_lesson.user_filename())
    

    def check_n_save_user_file(self, window):
        if self.user_file_mod:
            save_file = sg.popup_yes_no('The source file has been modified.\nDo you want to save it?', title=f'Modified {self.code_lesson.user_filename()}', grab_anywhere=True) #, modal=False)
            if save_file == 'Yes':
                self.save_user_file(window)
    
    def save_user_file(self, window):
        if self.user_file is not None:
            with open(self.user_file,'w') as f:
                f.write(self.values['-USER_TEXT-'])
            window['-USER_FILE-'].update(value=self.code_lesson.user_filename())
            self.user_file_mod = False
        
    def create_window(self):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        This editor is intentionally very simple. I orginally had the guidance as a second window
        pane but this wastes screen space and is annoying when you don't need the guidance.
        """
        window_width = 100
        column_width = int(window_width/2)
        menu_layout = [
                ['File',['&Save','---','Help','Exit']],
                ['Edit',['&Copy','Paste']],
            ]

        self.lesson_text = sg.Multiline(default_text='Lesson File', font=self.config.get('font'), enable_events=True, key='-LESSON_TEXT-', size=(window_width,10), expand_x=True, expand_y=True)
        self.user_text   = sg.Multiline(default_text='User File',   font=self.config.get('font'), enable_events=True, key='-USER_TEXT-',   size=(window_width,20), expand_x=True, expand_y=True)

        window_layout = [
            [sg.Menu(menu_layout)],
            [
              sg.Text('App File:', font=self.config.get('font')),
              sg.Text(self.NEW_FILE_STR, font=self.config.get('font'), size=(column_width,1), relief=sg.RELIEF_RAISED, border_width=1, justification='center', key='-USER_FILE-'),
              sg.Button('◄', font='arrow_font 11', border_width=0, pad=(2,0), key='-FILE_LEFT-'),
              sg.Button('►', font='arrow_font 11', border_width=0, pad=(2,0), key='-FILE_RIGHT-'),
              sg.Button('Lesson Completed', enable_events=True, key='-COMPLETED-', pad=(2,0), size=(15,0), tooltip='Select after you complete all lesson files and exercises ')
            ],
            [self.user_text],
            [
              sg.Text('Exercise:', font=self.config.get('font')),# size=(window_width-10,1)
              sg.Text(self.code_lesson.exercise_id(), font=self.config.get('font'), size=(5,1), relief=sg.RELIEF_RAISED, border_width=1, justification='center', key='-EXERCISE-'),
              sg.Button('▼', font='arrow_font 11', border_width=0, pad=(2,0), key='-CODE_DOWN-'),
              sg.Button('▲', font='arrow_font 11', border_width=0, pad=(2,0), key='-CODE_UP-'),
              sg.Button('Instructions', enable_events=True, key='-INSTRUCTIONS-', pad=(2,0), size=(10,0), tooltip='Exercise instructions')
            ],
            [self.lesson_text]
            ]

        window = sg.Window(self.title, window_layout, resizable=True, margins=(0,0), return_keyboard_events=True, finalize=True, modal=True)
        return window
       
                
    def gui(self):
    
        window = self.create_window()
        
        self.read_lesson_file()
        self.write_lesson_window(window)
        self.open_user_file(window)

        prev_encoded_event = None
        read_with_timeout  = False
        while True:

            self.event, self.values = window.read() # (timeout=50) - Using a timeout stops the control scheme from working

            if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None:
                if self.user_file_mod:
                    if self.values is None:
                        sg.popup('The source file has been modified. Your changes will not be saved due to a bug. In the future use Ctrl-s to save your file prior to exiting', title=f'Modified {self.code_lesson.user_filename()}', grab_anywhere=True) #, modal=False)
                    else:
                        self.check_n_save_user_file(window)
    
                break
            elif self.event == '-COMPLETED-':
                self.code_lesson.json.set_complete(True)
                self.lesson_completed = True
                break
            
            # The goal is to capture control-key events in a portable way so I avoided the tkinter event
            # binding method. However, I've only tested this on tkinter so it may not be portable.
            # When a control-key sequence is pressed two events are generated. key followed by a delayed
            # Control_L:37. This logic saves the key and creates a new encoded string.
            # Seems like a hack, but it works!
            
            encoded_event = str(str(self.event).encode('utf-8'))  
                    
            if encoded_event in ("b'Control_L:37'"):
                if prev_encoded_event is not None:
                    if ':' in prev_encoded_event:
                        encoded_event = encoded_event.split(':')[0] + ':' + prev_encoded_event.split(':')[1]
                    prev_encoded_event = None
             
            ### File Menu ###          
           
            if encoded_event in ("b'Help'"):
                self.help_text.display()
            
            elif encoded_event in ("b'Save'","b'Control_L:39'"): # s=39
                self.save_user_file(window)

            ### Edit Menu & User File Mods ###
                
            elif encoded_event in ("b'Copy'", "b'Control_L:54'"): # c=54
                selection = window['-LESSON_TEXT-'].Widget.selection_get()
                sg.clipboard_set(selection)
                
            elif encoded_event in ("b'Paste'", "b'Control_L:55'"): # v=55
                # I'm not sure why but the Widget.delete() call pastes what is in sg.clipboard
                # An exception is raised if no selection is made but the paste still works
                try:
                    window['-USER_TEXT-'].Widget.delete("sel.first", "sel.last")
                except:
                    pass
                #text = sg.clipboard_get()
                #window['-USER_TEXT-'].Widget.insert("insert", text)

            elif encoded_event in ("b'-USER_TEXT-'",):
                if not self.user_file_mod: 
                    self.user_file_mod = True
                    new_title = '*' + window['-USER_FILE-'].get()
                    window['-USER_FILE-'].update(value=new_title)
                
            ### Navigate Lesson Files and Exercises ###

            elif encoded_event in ("b'-FILE_LEFT-'",):
                self.code_lesson.decrement_file()
                self.update_file_displays(window)
                
            elif encoded_event in ("b'-FILE_RIGHT-'",):
                self.code_lesson.increment_file()
                self.update_file_displays(window)
                
            elif encoded_event in ("b'-CODE_DOWN-'",):
                self.code_lesson.increment_exercise()
                self.write_lesson_window(window)
                
            elif encoded_event in ("b'-CODE_UP-'",):
                self.code_lesson.decrement_exercise()
                self.write_lesson_window(window)
                
            elif encoded_event in ("b'-INSTRUCTIONS-'",):
                sg.popup(self.code_lesson.exercise_instructions(), title=f'{self.code_lesson.exercise_id()} Instructions', line_width=132)

            prev_encoded_event = encoded_event

            
        window.close()
        
    def execute(self):
        self.gui()
        self.code_lesson.json.update()
        return self.lesson_completed


###############################################################################

if __name__ == '__main__':

    tutorial_dir = None
    if len(sys.argv) > 1:
        # argv is relative path from gnd-sys/app
        # Since executing from gnd-sys/app/tools need to go back one level
        # Example from tools directory: python3 appcodetutorial.py ../../usr/apps/hello/tutorial
        tutorial_dir = compress_abs_path(os.path.join(os.getcwd(),'..',sys.argv[1]))
        temp_file = os.path.join(os.getcwd(),'temp.txt')
        #with open('/home/osk/sandbox/cayg/temp.txt','w') as f:
        #    f.write(tutorial_dir)
    else:
        tutorial_dir = compress_abs_path(os.path.join(os.getcwd(),'../../templates/hello-world/tutorial'))
        tutorial_dir = compress_abs_path(os.path.join(os.getcwd(),'../../../usr/apps/hello-1/tutorial'))
    print ("Main: tutorial_dir = " + tutorial_dir)
            
    tutorial = CodeTutorial(tutorial_dir)
    tutorial.execute()
    
    

