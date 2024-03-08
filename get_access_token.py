import sys
from google.oauth2 import service_account
from google.auth.transport.requests import Request

def get_access_token(service_account_file):
    credentials = service_account.Credentials.from_service_account_file(
        service_account_file,
        scopes=["https://www.googleapis.com/auth/cloud-platform"],
    )
    credentials.refresh(Request())
    return credentials.token

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python get_access_token.py <path_to_service_account_json>")
        sys.exit(1)
    service_account_file = sys.argv[1]
    token = get_access_token(service_account_file)
    print(token)
