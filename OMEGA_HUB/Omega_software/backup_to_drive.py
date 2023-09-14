"""
File: backup_to_drive.py
Description: This script backs up the data folder to Google Drive.
Author: Sami Kaab
Date: 2023-07-05
"""

import os
import os.path
import shutil
import logging
import requests
from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError
from googleapiclient.http import MediaFileUpload
from google.oauth2 import service_account

# Constants
SCOPES = ["https://www.googleapis.com/auth/drive"]
DRIVE_ROOT_FOLDER = "WMORE_DATA"
DATA_DIR = "data"
BACKUP_DIR = "BackedUp"

def setup_logging():
    logfilename = '/root/WMORE/logs/WMORE.log'
    # create file if it doesn't exist
    if not os.path.exists(logfilename):
        with open(logfilename, 'w') as f:
            pass
    #setup logging  
    logging.basicConfig(filename=logfilename, level=logging.INFO, format='%(asctime)s %(levelname)s %(name)s %(message)s')

def get_credentials():
    """
    Get the user credentials for Google Drive.

    Returns:
        The user credentials for Google Drive.
    """
    # Check if a token file exists and load the credentials from it if it does
    creds = None
    creds = service_account.Credentials.from_service_account_file("credentials.json", scopes=SCOPES)

    return creds

def is_internet_available():
    """Check if an internet connection is available."""
    try:
        requests.get("http://google.com", timeout=5)
        # logging.info("Internet access available")
        return True
    except:
        # logging.error("No internet access available")
        return False

def delete_all():
    # Set up the service account credentials
    credentials = get_credentials()

    # Build the Google Drive API client
    drive_service = build("drive", "v3", credentials=credentials)

    # List all files
    response = drive_service.files().list().execute()
    files = response.get("files", [])

    # Iterate through the files and delete them
    for file in files:
        file_id = file["id"]
        file_name = file["name"]
        drive_service.files().delete(fileId=file_id).execute()
        print(f"Deleted file: {file_name}")
        
def download_all():
    credentials = get_credentials()
   # Build the Google Drive API client
    drive_service = build("drive", "v3", credentials=credentials)

    # Retrieve file metadata with parent information
    response = drive_service.files().list(fields="files(id, name, mimeType, parents)").execute()
    files = response.get("files", [])

    # Iterate through the files and download them
    for file in files:
        file_id = file["id"]
        file_name = file["name"]
        parents = file.get("parents", [])
        mime_type = file["mimeType"]

        # Create the folder structure on the local system
        folder_path = ""
        for parent_id in parents:
            parent_response = drive_service.files().get(fileId=parent_id, fields="name").execute()
            parent_name = parent_response.get("name", "")
            folder_path = os.path.join(parent_name, folder_path)
            os.makedirs(folder_path, exist_ok=True)

        # Download the file
        if "application/vnd.google-apps" in mime_type:
            print(f"Skipping export for file: {file_name}. It is not a supported Google Docs Editors file.")
            continue

        request = drive_service.files().get_media(fileId=file_id)
        file_content = request.execute()

        file_path = os.path.join(folder_path, file_name)
        with open(file_path, "wb") as f:
            f.write(file_content)

        print(f"Downloaded file: {file_path}")

def create_folder(service, name, parent_folder_id=None):
    # check if folder exists and return id if it does
    query = f"name='{name}' and mimeType='application/vnd.google-apps.folder'"
    if parent_folder_id:
        # if parent_folder_id is specified, add it to the query
        query += f" and '{parent_folder_id}' in parents"

    response = service.files().list(q=query, spaces='drive', fields='files(id)').execute()
    if len(response['files']) > 0:
        return response['files'][0]['id']
    
    
    folder_metadata = {
        'name': name,
        'mimeType': 'application/vnd.google-apps.folder'
    }
    if parent_folder_id:
        folder_metadata['parents'] = [parent_folder_id]

    folder = service.files().create(body=folder_metadata, fields='id').execute()
    return folder['id']

def upload_file(service, file_path, parent_folder_id):
    # check if file exists and return id if it does
    query = f"name='{os.path.basename(file_path)}' and '{parent_folder_id}' in parents"
    response = service.files().list(q=query, spaces='drive', fields='files(id)').execute()
    if len(response['files']) > 0:
        return response['files'][0]['id']
    
    file_metadata = {
        'name': os.path.basename(file_path),
        'parents': [parent_folder_id]
    }

    media = MediaFileUpload(file_path, mimetype='application/octet-stream')
    file = service.files().create(body=file_metadata, media_body=media, fields='id').execute()
    logging.info(f"Backed up file: {file_path}")
    return file['id']



def clone_folder_structure(service, local_folder, parent_folder_id=None):
    
    folder_name = os.path.basename(local_folder)
    folder_id = create_folder(service, folder_name, parent_folder_id)

    for item in os.listdir(local_folder):
        item_path = os.path.join(local_folder, item)
        if os.path.isdir(item_path): # if item is a folder, recursively clone it
            clone_folder_structure(service, item_path, folder_id)
        else: # if item is a file, upload it
            file_exists = check_if_already_exist(item_path, service, folder_id)
            if not file_exists:
                upload_file(service, item_path, folder_id)
            else:
                print(f"{item_path} already exists")

def upload():
    # Get the user credentials for Google Drive
    creds = get_credentials()
    # Build the Google Drive API client with the user credentials
    service = build("drive", "v3", credentials=creds)
    drive_root_folder_id = create_folder(service, DRIVE_ROOT_FOLDER)
    clone_folder_structure(service, DATA_DIR, drive_root_folder_id)
    
def check_if_already_exist(file_path, service, parent_folder_id):
    """
    Check if a file with the same name and size as 'file_path' already exists in the specified Google Drive folder.

    Args:
        file_path (str): The local file path to check.
        service: The Google Drive API service.
        parent_folder_id (str): The ID of the parent folder to check within.

    Returns:
        bool: True if a file with the same name and size exists, False otherwise.
    """
    file_name = os.path.basename(file_path)
    # Get the file size
    file_size = os.path.getsize(file_path)

    # Create a query to search for the file by name and parent folder
    query = f"name='{file_name}' and '{parent_folder_id}' in parents"

    # Execute the query and retrieve the list of matching files
    response = service.files().list(q=query, spaces='drive', fields='files(name, size)').execute()
    matching_files = response.get('files', [])

    # Check if any of the matching files have the same name and size
    for matching_file in matching_files:
        if matching_file['name'] == file_name and int(matching_file['size']) == file_size:
            return True

    return False

     

if __name__ == "__main__":
#     # setup_logging()
#     # os.makedirs(BACKUP_DIR, exist_ok=True)
    
    # delete_all()
    # exit()

    if is_internet_available():
        upload()