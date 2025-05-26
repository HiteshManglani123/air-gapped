import paramiko
import time
import sys

def ssh_connect(hostname, username, password=None, key_filename=None, port=22):
    """
    Connect to an SSH server using either password or key-based authentication.
    
    Args:
        hostname (str): The hostname or IP address of the SSH server
        username (str): The username for authentication
        password (str, optional): Password for authentication
        key_filename (str, optional): Path to the private key file
        port (int, optional): SSH port number (default: 22)
    
    Returns:
        tuple: (SSHClient, bool) - SSH client object and connection status
    """
    try:
        # Create SSH client
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        
        # Connect to the server
        if key_filename:
            ssh.connect(hostname=hostname,
                       username=username,
                       key_filename=key_filename,
                       port=port)
        else:
            ssh.connect(hostname=hostname,
                       username=username,
                       password=password,
                       port=port)
        
        print(f"Successfully connected to {hostname}")
        return ssh, True
        
    except paramiko.AuthenticationException:
        print("Authentication failed. Please check your credentials.")
        return None, False
    except paramiko.SSHException as ssh_exception:
        print(f"SSH error: {ssh_exception}")
        return None, False
    except Exception as e:
        print(f"Error: {e}")
        return None, False

def execute_command(channel, command):
    """
    Execute a command in the interactive shell session.
    
    Args:
        channel: The SSH channel
        command (str): Command to execute
    
    Returns:
        str: Command output
    """
    try:
        channel.send(command + '\n')
        time.sleep(1)  # Give the command time to execute
        output = channel.recv(4096).decode()
        return output
    except Exception as e:
        print(f"Error executing command: {e}")
        return None

def main():
    # Predefined connection details
    hostname = #"HOSTNAME/HOST_IP_HERE"
    username = #"USERNAME_HERE"
    password = #"PASSWORD_HERE"
    
    print(f"Attempting to connect to {hostname}...")
    ssh, success = ssh_connect(hostname, username, password=password)
    
    if success:
        # Create an interactive shell session
        channel = ssh.invoke_shell()
        time.sleep(1)  # Wait for the shell to be ready
        
        # Navigate to Desktop/Hitesh directory and run commands
        print("Navigating to Desktop/Hitesh directory...")
        commands = [
            "cd ~/Desktop/Hitesh", #Change the Hitesh to the directory where's the receiver software
            "pwd",  # Print current directory to verify
            "make",
            "./receiver"
        ]
        
        for cmd in commands:
            output = execute_command(channel, cmd)
            if output:
                print(f"Output: {output.strip()}")
            time.sleep(1)  # Wait between commands
        
        print("\nCommands executed. Ready for additional commands.")
        
        while True:
            command = input("Enter command to execute (or 'exit' to quit): ")
            if command.lower() == 'exit':
                break
                
            output = execute_command(channel, command)
            if output:
                print("Output:", output)
        
        channel.close()
        ssh.close()
        print("Connection closed.")

if __name__ == "__main__":
    main()
    