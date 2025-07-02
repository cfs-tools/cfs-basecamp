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
      Manage reading/writing cFE files
      
    Notes:
      1. There's no need to differentiate between table files and other
         cFE binary files because the EDS is used to process all binary
         files.
      2. Includes an API for using EDS definitions to read/write file
         data. Files are not part of the EDS <RequiredInterfaceSet> spec
         which is used for command & telemetry messages. The file EDS
         container type definitions can't be directly accessed EdsLib.
         They must be accessed from the RequiredInterface and this interface
         is defined from a Software Bus perspective. The workaround is 
         to define binary files in a pseudo telemetry packet.
      3. Pseudo telemetry messages are used because command function codes
         complicate using them. However telemetry messages create the problem
         that existing CfeTarget classes don't have methods for loading
         telemetry values. Therefore some functionality has been added to
         the CfeFile that may be suitable for higher level classes.
      4. By convention EDS table file interface names must end with "TBL_FILE" 
"""

import sys
import time
import os
import socket
import configparser
import io


if __name__ == '__main__' or 'cfsinterface' in os.getcwd():
    sys.path.append('..')
    from edsmission    import EdsMission
    from edsmission    import CfeEdsTarget
    from cfeconstants  import Cfe
else:
    from .edsmission    import EdsMission
    from .edsmission    import CfeEdsTarget
    from .cfeconstants  import Cfe


###############################################################################
"""
The cFS defines the binary header file in cfe_fs.xml:

    typedef struct
    {
        uint32  ContentType;
        uint32  SubType;
        uint32  Length;
        uint32  SpacecraftID;
        uint32  ProcessorID;
        uint32  ApplicationID;

        uint32  TimeSeconds;
        uint32  TimeSubSeconds;

        char    Description[CFE_FS_HDR_DESC_MAX_LEN];  // 32
    } CFE_FS_Header_t;
    64 bytes

The cFS defines the binary header file in cfe_tbl.xml:
    typedef struct
    {
        uint32                   Reserved;
        uint32                   Offset;
        uint32                   NumBytes;
        char                     TableName[CFE_MISSION_TBL_MAX_FULL_NAME_LEN];  // 40 bytes
    } CFE_TBL_File_Hdr_t;
    52 bytes

"""
            
###############################################################################

class CfeFile(CfeEdsTarget):
    """
    This class can be used for all binary files because the files are defined in 
    EDS and a "pseudo telmeetry" topic ID is used to identify the EDS messages
    that defined the file's contents. See the file prologue notes for more details.
    
    Assumes cFE files are of a reasonable size and the entire file can be read
    into memory.    
    """

    def __init__(self, mission, target):
        super().__init__(mission, target, EdsMission.TELEMETRY_IF) # Files are defined as telemetry messages

        self.path_filename = None
        self.data_loaded = False
        self.tbl_data_array = []   # 2 column matrix with each row containing EDS parameter string and its value
        self.topic_name = None
        self.topic_dict = self.eds_mission.get_tbl_topic_dict()
        print(f'self.topic_dict: {self.topic_dict}')
        
    def extract_data(self, base_object, base_name):
        """
        Inputs:
            base_object: The EDS object to iterate over
            base_name:   The base name for the sub-entities printed to the screen

        Recursive function that iterates over an EDS object and prints the contents of
        the sub-entries to the screen
        """
        
        # Array display string
        if (self.eds_mission.lib_db.IsArray(base_object)):
            #print("@DEBUG@extract_data()-array: base_object = " + str(base_object))
            #print("@DEBUG@extract_data()-array: base_name = " + str(base_name))
            for i in range(len(base_object)):
                self.extract_data(base_object[i], f"{base_name}[{i}]")
        # Container display string
        elif (self.eds_mission.lib_db.IsContainer(base_object)):
            #print("@DEBUG@extract_data()-container: base_object = " + str(base_object))
            #print("@DEBUG@extract_data()-container: base_name = " + str(base_name))
            for item in base_object:
                self.extract_data(item[1], f"{base_name}.{item[0]}")
        # Everything else (number, enumeration, string, etc.)
        else:
            #print('{:<60} = {}'.format(base_name, base_object))
            self.tbl_data_array.append([base_name,base_object])


    def read(self, path_filename, topic_name):
        """
        TODO: Add pseudo telemetry CCSDS header with topic_id
        """
        self.path_filename = path_filename
        self.topic_name = topic_name 
        self.data_loaded = False
        self.file_data = None
        self.tbl_data_array = []
        self.err_str = None
        
        if path_filename not in (None,''):
            if os.path.isfile(path_filename):
                try:              
                    with open(self.path_filename, 'rb') as file:
                        self.file_data = file.read()
                    self.file_len       = len(self.file_data)
                    self.file_data_list = list(self.file_data)
                    print(f'self.file_len: {self.file_len}')
                    
                    """
                    Create a pseudo telemetry packet
                      1. Create a CCSDS header with topic ID and length (add 5 to file table length)
                      2. Add CCSDS header len (12 bytes) to file header data length
                      3. Concatentate CCSDS header and table file data
                    """
                    print(f'len: {self.file_data[8]}, {self.file_data[9]}, {self.file_data[10]}, {self.file_data[11]}')
                    file_hdr_len_lo = self.file_data[10] << 8 | self.file_data[11]
                    file_hdr_len_hi = self.file_data[8]  << 8 | self.file_data[9]
                    file_hdr_len    = file_hdr_len_hi << 16 | file_hdr_len_lo
                    print(f'file_hdr_len: {file_hdr_len}')  
                    topic_id    = self.topic_dict[topic_name] + 3
                    tlm_msg_len = len(self.file_data) + 5
                    print(f'topic_id: {topic_id}, tlm_msg_len: {tlm_msg_len}') 
                    tlm_msg_len = list(tlm_msg_len.to_bytes(2,byteorder='big'))
                    print(f'bytes(tlm_msg_len): {tlm_msg_len}')
                    tlm_msg_len_lo = tlm_msg_len[1]
                    tlm_msg_len_hi = tlm_msg_len[0]
                    print(f'tlm_len_hi: {tlm_msg_len_hi}, tlm_len_lo: {tlm_msg_len_lo}')
                    #                                 Sequence    Length                          Seconds                 SubSecs
                    self.ccsds_hdr = [0x08, topic_id, 0xD7, 0xC2, tlm_msg_len_hi, tlm_msg_len_lo, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05]
                    
                    self.file_len = self.file_len + 12
                    self.file_len = list(self.file_len.to_bytes(4,byteorder='big'))
                    print(f'bytes(self.file_len): {self.file_len}')                   
                    self.file_data_list[8]  = self.file_len[0]
                    self.file_data_list[9]  = self.file_len[1]
                    self.file_data_list[10] = self.file_len[2]
                    self.file_data_list[11] = self.file_len[3]
                    
                    self.pseudo_tlm_msg = bytes(self.ccsds_hdr) + bytes(self.file_data_list)
                    print(f'self.pseudo_tlm_msg: {self.pseudo_tlm_msg}')
                    with open('../../flt-file-server/debug.tbl', 'wb') as binary_file:
                         binary_file.write(self.pseudo_tlm_msg)
                    eds_entry, eds_obj = self.eds_mission.decode_message(self.pseudo_tlm_msg)
                    self.extract_data(eds_obj, eds_entry.Name)
                    self.data_loaded = True
                    print('Exit read()')
                except Exception as e:
                    print(f'Exception: {e}')                    
                    self.err_str = str(e)
                    
        else:
            # TODO: Remove testing backdoor
            self.tbl_data_array = self.reverse_eng()
            
        return (self.err_str, self.tbl_data_array)


    def write(self):
        """
        """
        return self.write_file(self.path_filename)
        

    def write_file(self, path_filename):
        """
        """
        print(f'\n>>>>write_file({path_filename})')
        self.err_str = None
        try:
            byte_data = self.create_byte_data()
            byte_data_list = list(byte_data)
            # Set file header length back to 
            # TODO: Use EDS to get file header length & resolve what the value should be. 
            byte_data_list[20] = 0
            byte_data_list[21] = 0
            byte_data_list[22] = 0
            byte_data_list[23] = 64
            byte_data = bytes(byte_data_list[12:])
            with open(path_filename, 'wb') as binary_file:
                binary_file.write(byte_data)
        except Exception as e:    
            self.err_str = str(e)
        return self.err_str


    def update_data_array(self, row, col, value):
        self.tbl_data_array[row][col] = value
        return self.tbl_data_array

        
    def reverse_eng(self):
        """
        Basecamp's FILE_MGR System Volume table is used 
        
            typedef struct
            {
               
               uint32  State; 
               char    Name[OS_MAX_PATH_LEN];  Defaults to 64

            } FILESYS_Volume_t;


            typedef struct{
               
               FILESYS_Volume_t Volume[FILE_MGR_FILESYS_TBL_VOL_CNT];  Defaults to 4

            } FILESYS_TblData_t;
            272 bytes

    
        Deduced meaning of error codes for eds_object = eds_entry(EdsLib.PackedObject(raw_message))

            RuntimeError: Error -6 unpacking bytes object   # Raw byte length doesn't match length in header 
            RuntimeError: Error -10 unpacking bytes object  # Pass length check length but values/type mismatch?
                                                            # In this case file header length was being validated
                                                            # against the byte array length
        """

        CFE_ES_HK = [
            0x08, 0x40, 0xD7, 0xC2, 0x00, 0x96, 0x00, 0x10, 0x38, 0x3F, 0x00, 0x1E, 0x00, 0x00, 0xD2, 0x46,  # CCSDS Headers (12 bytes)
            0x06, 0x07, 0x63, 0x00, 0x05, 0x00, 0x00, 0xFF, 0x01, 0x04, 0x00, 0x63, 0x00, 0x00, 0x0C, 0x00,  # 145 byte payload
            0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x4E, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,  # 0x96 = 150 (157 - 7)
            0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]

        FILE_MGR_SYS_TBL_TLM = [                                                                             # CCSDS Headers (12 bytes)
            0x08, 0x67, 0xD7, 0xC2, 0x01, 0x8D, 0x00, 0x10, 0x38, 0x3F, 0x00, 0x1E,                          # APID 0x67 (61+39+3)
                                                                                                             # Length 0x0189 (404-7=397)
                                                                                                             # cFE File Header (64 bytes)
            0x63, 0x46, 0x45, 0x31, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x94, 0x00, 0x00, 0x00, 0x42,  # cFE1.... ...@...B
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x46, 0x69, 0x6c, 0x65, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x20, 0x76, 0x6f, 0x6c, 0x75,  # File.sys tem.volu
            0x6d, 0x65, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # mes..... ........
                                                                                                             # Table Header (56 bytes)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x46, 0x49, 0x4c, 0x45,  # ........ ....FILE
            0x5f, 0x4d, 0x47, 0x52, 0x2e, 0x46, 0x69, 0x6c, 0x65, 0x53, 0x79, 0x73, 0x54, 0x62, 0x6c, 0x00,  # _MGR.Fil eSysTbl.
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                  # ........
                                                                                                             # Table Data (272 bytes)            
            0x00, 0x00, 0x00, 0x01, 0x2f, 0x72, 0x61, 0x6d,                                                  # ..../ram
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x2f, 0x62, 0x6f, 0x6f, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # /boot... ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x2f, 0x61, 0x6c, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ..../alt ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]                                                  # ........

        FILE_MGR_SYS_FILE_HDR = [                                                                            # CCSDS Headers (12 bytes)                                                                                                             
            0x08, 0x67, 0xD7, 0xC2, 0x00, 0x45, 0x00, 0x10, 0x38, 0x3F, 0x00, 0x1E,                          # APID 0x67 (61+39+3)
                                                                                                             # cFE File Header (64 bytes)
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x00, 0x00, 0x42,  # cFE1.... ...@...B
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x46, 0x69, 0x6c, 0x65, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x20, 0x76, 0x6f, 0x6c, 0x75,  # File.sys tem.volu
            0x6d, 0x65, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]  # mes..... ........
            
        FILE_MGR_SYS_TBL_HDR = [                                                                             # CCSDS Headers (12 bytes)
            0x08, 0x67, 0xD7, 0xC2, 0x00, 0x3D, 0x00, 0x10, 0x38, 0x3F, 0x00, 0x1E,                          # APID 0x67 (61+39+3)
                                                                                                             # Table Header (56 bytes)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20, 0x46, 0x49, 0x4c, 0x45,  # ........ ....FILE
            0x5f, 0x4d, 0x47, 0x52, 0x2e, 0x46, 0x69, 0x6c, 0x65, 0x53, 0x79, 0x73, 0x54, 0x62, 0x6c, 0x00,  # _MGR.Fil eSysTbl.
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]                                                  # ........

        FILE_MGR_SYS_TBL_DATA = [                                                                            # CCSDS Headers (12 bytes)
            0x08, 0x67, 0xD7, 0xC2, 0x01, 0x15, 0x00, 0x10, 0x38, 0x3F, 0x00, 0x1E,                          # APID 0x67 (61+39+3)
                                                                                                             # Table Data (272 bytes)
            0x00, 0x00, 0x00, 0x01, 0x2f, 0x72, 0x61, 0x6d,                                                  # ..../ram
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x2f, 0x62, 0x6f, 0x6f, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # /boot... ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x2f, 0x61, 0x6c, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ..../alt ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # ........ ........
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]                                                  # ........

        

        #self.save_file('file_mgr_sys_pseudo.tbl', bytes(FILE_MGR_SYS_TBL_TLM))
        
        #eds_entry, eds_obj = self.eds_mission.decode_message(bytes(CFE_ES_HK))
        eds_entry, eds_obj = self.eds_mission.decode_message(bytes(FILE_MGR_SYS_TBL_TLM))
        
        print("@REVERSE: eds_entry type %s, value: %s" % (str(type(eds_entry)), str(eds_entry)))
        print("@REVERSE: eds_obj type %s, value: %s" % (str(type(eds_obj)), str(eds_obj)))
        
        self.tbl_data_array = []
        self.extract_data(eds_obj, eds_entry.Name)

        eds_entry = self.eds_mission.get_database_named_entry('FILE_MGR/FileSysTblFile')
        print("@REVERSE: eds_entry type: %s\n@@dir: %s\n@@value: %s" % (str(type(eds_entry)), str(dir(eds_entry)), str(eds_entry)))
        eds_obj = eds_entry()
        print("@REVERSE: eds_obj type: %s\n@@dir: %s\n@@value: %s" % (str(type(eds_obj)), str(dir(eds_obj)), str(eds_obj)))
        pri_hdr = eds_obj.CCSDS
        print("@REVERSE: pri_hdr type: %s\n@@dir: %s\n@@value: %s" % (str(type(pri_hdr)), str(dir(pri_hdr)), str(pri_hdr)))        
        sec_hdr = eds_obj.Sec
        print("@REVERSE: sec_hdr type: %s\n@@dir: %s\n@@value: %s" % (str(type(sec_hdr)), str(dir(sec_hdr)), str(sec_hdr)))        
        payload = eds_obj.Payload
        print("@REVERSE: payload type: %s\n@@dir: %s\n@@value: %s" % (str(type(payload)), str(dir(payload)), str(payload)))   
        
        return self.tbl_data_array


    def load_payload_entry_value(self, payload_name, payload_eds_entry, payload_type, payload_list):
        """
        """
        #print(f'payload_name={payload_name}, payload_eds_entry={payload_eds_entry}, payload_type={payload_type}, payload_list={payload_list}')
        #todo: Add type check error reporting
        
        value = self.tbl_data_dict[payload_name]
        return value

        
    def create_byte_data(self):
        """
        Create a binary byte data array from data self.tbl_data_array
        """

        self.tbl_data_array
        #print(f'\n***self.tbl_data_array: {self.tbl_data_array}\n')
        #for entry in self.tbl_data_array:
        #    print(f'{entry}')
        self.tbl_data_dict = {entry[0][entry[0].find('.')+1:] : entry[1] for entry in self.tbl_data_array} 
        #print(f'\n***tbl_data_dict: {self.tbl_data_dict}')
               
        eds_id = self.eds_mission.get_eds_id_from_topic('FILE_MGR/Application/FILE_SYS_TBL_FILE')
        self.tlm_entry = self.eds_mission.get_database_entry(eds_id)
        self.tlm_obj   = self.tlm_entry()
            
        #payload = self.eds_mission.get_topic_payload('FILE_MGR/Application/FILE_SYS_TBL_FILE')
        #print(f'payload: {payload}')
        payload_entry = self.eds_mission.get_database_named_entry('FILE_MGR/FileSysTblFile_Payload')
        payload = payload_entry()
        self.payload_struct = self.get_payload_struct(payload_entry, payload, 'Payload')
        
        #print(f'\n***payload_entry:\n{payload_entry}\npayload:\n{payload}')
        #print(f'\n***payload_struct:\n{self.payload_struct}')

        eds_payload = self.set_payload_values(self.payload_struct)
        payload = payload_entry(eds_payload)                           
        self.tlm_obj['Payload'] = payload
                            
        packed_tlm = self.eds_mission.get_packed_obj(self.tlm_obj)
        packed_tlm_bytes = bytes(packed_tlm)
        #print(f'\n***packed_tlm_bytes:\n{packed_tlm_bytes}')
        
        return packed_tlm_bytes
        

###############################################################################

if __name__ == '__main__':
    """
    """

    config = configparser.ConfigParser()
    config.read('../basecamp.ini')

    MISSION         = config.get('CFS_TARGET', 'MISSION_EDS_NAME')
    CFS_TARGET      = config.get('CFS_TARGET', 'CPU_EDS_NAME')
    
    cfe_file = CfeFile(MISSION, CFS_TARGET)
    cfe_file.reverse_eng()
    
    #cfe_cmd_file.process_file('bogus_file', 'FILE_MGR/FileSys_Tbl') # CFE_ES/QueryOneCmd'
    #cfe_cmd_file.process_file('bogus_file', 'CFE_ES/QueryOneCmd', {'Application': 'APP_C_DEMO'})