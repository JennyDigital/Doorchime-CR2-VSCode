import os
 
# CUBE_PROGRAMMER_CLI_PATH = '/opt/st/stm32cubeclt_1.18.0/STM32CubeProgrammer/bin'
FILE_TO_DOWNLOAD_NAME = './chime128k.elf'
CONNECTION_PORT = 'swd'
 
def main():
    os.system(f"STM32_Programmer_CLI -c port={CONNECTION_PORT} -w {FILE_TO_DOWNLOAD_NAME} -v")
    os.system(f"STM32_Programmer_CLI -c port={CONNECTION_PORT} -hardRst")
 
if __name__ == '__main__':
    main()
