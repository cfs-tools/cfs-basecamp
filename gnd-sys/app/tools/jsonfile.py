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
        Provide JSON base class.
"""

from abc import ABC, abstractmethod
import sys
import time
import os
import json
from datetime import datetime


###############################################################################

class JsonFile():
    """
    Abstract base class to manage JSON files used to store state in a consistent
    manner. This is designed for Basecamp as opposed to general utility so it's up
    to the user to protect against errors. JSON key constants should all be used
    within the Json classes to localize impacts due to JSON key changes.
    """
    def __init__(self, json_file):

        self.file = json_file
        f = open(json_file)
        self.json = json.load(f)
        f.close()
        
    def title(self):
        return self.json['title']

    def objective(self):
        return self.json['objective']

    def description(self):
        return self.json['description']

    def reset(self):
        """
        Reset only applies when a JSON file is being used to maintain state
        across executions.
        """
        self.json['time-stamp'] = ""
        self.reset_child()
        self.write_file()

    @abstractmethod
    def reset_child(self):
        raise NotImplementedError
            
    def update(self):
        self.json['time-stamp'] = datetime.now().strftime("%m/%d/%Y-%H:%M:%S")
        self.write_file()
    
    def write_file(self):
        with open(self.file, "w") as outfile:
            json.dump(self.json, outfile, indent=2)
    

