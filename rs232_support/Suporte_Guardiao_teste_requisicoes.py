
try:

    import serial
    from Crypto.Cipher import AES
    from Crypto import Random
    import time
    import random
    import datetime
    import pytz
    import pandas as pd

except Exception as erro:
    print(" erro de importacao em : %s" % erro)
    exit(0)

path_csv = "log_info.csv"

def logs_csv(msg):
      info = {
               'Data':[],
               'Hora':[],
               'Timestamp':[],
               'Tipo':[],
               'Classe':[]
            }
      for fist_log_pos in range(0,len(msg),10):
         data_timestamp = 0
         n = fist_log_pos

         if msg[n] == 160 or msg[n] == 172:
            type_log = 'Relé'

            atack_type = msg[n+5] & 0x07
            if atack_type == 1:
               class_log = 'Normal'
            elif atack_type == 2:
               class_log = 'Térmico'
            elif atack_type == 4:
               class_log = 'Vibração'
            else:
               class_log = '0'
            
         elif msg[n] == 163 or msg[n] == 175:
            type_log = 'Event'
            if ( (((msg[n+5] & 0x01) == 0)) & (( (msg[n+5] >> 4) & 0x01) == 0) ):
               class_log = 'Case/IR'
            elif (msg[n+5] & 0x01) == 0:
               class_log = 'Case'
            elif ( (msg[n+5] >> 4) & 0x01) == 0:
               class_log = 'IR'
            elif (msg[n+5] & 0x01) == 1:
               class_log = 'Case fechado'
            elif ( (msg[n+5] >> 4) & 0x01) == 1:
               class_log = 'Ir conectado'
            else:
               class_log = '0'

         elif msg[n] == 161 or msg[n] == 173:
            type_log = 'Ultra'
            class_log = str(msg[n+5])


         else:
            type_log = '0'
            class_log = '0'
                  
         data_timestamp = msg[n+4] << 24 | msg[n+3] << 16 | msg[n+2] << 8 | msg[n+1]
         timestamp = data_timestamp

         if data_timestamp == 0:
            date = '0'
            hour = '0'
         else:
            timezone = pytz.timezone('America/Manaus')
            date = datetime.datetime.fromtimestamp(data_timestamp).strftime('%d-%m-%Y')
            hour = datetime.datetime.fromtimestamp(data_timestamp).astimezone(timezone).strftime('%H:%M:%S')

         info['Tipo'].append(type_log)
         info['Classe'].append(class_log)
         info['Timestamp'].append(data_timestamp)
         info['Data'].append(date)
         info['Hora'].append(hour)
      
      # for key in info:
      #    info[key].reverse()
      df = pd.DataFrame(info)
      df.to_csv(path_csv, index=False)
      print(f"CSV finalizado: {path_csv}")

def timestamp_update():
      try:
         response = s.read(5) 
         print("response:",response)
      except Exception:
         print('Nao recebeu o OK para o timestamp')
         exit(0)
      if response!=b'R06OK':
         raise Exception("Recebeu uma resposta errada para a preparação do timestamp")

      data_timestamp = int(time.time()) 
      print(f"Timestamp enviado: {data_timestamp}")
      # byte1 byte2 byte3 byte4  (mais -> menos) significativo
      byte1 = chr((data_timestamp >> 24) & 0xFF)
      byte2 = chr((data_timestamp >> 16) & 0xFF)
      byte3 = chr((data_timestamp >> 8) & 0xFF)
      byte4 = chr(data_timestamp & 0xFF)
      data_timestamp = byte4 + byte3 + byte2 + byte1 # Guardiao precisa receber do menos para o mais significativo
      
      #Envia o timestamp criptografado
      iv = Random.new().read(AES.block_size)
      cipher = AES.new(key, AES.MODE_CFB, iv)
      crypt_answer = iv + cipher.encrypt(data_timestamp.encode('latin-1'))
      s.write(crypt_answer)
      
      try:
         response = s.read(5) 
         # print("response:",response)
      except Exception:
         print('Atualização do timestamp falhou')
         exit(0)
      if response!=b'R06TP':
         raise Exception("Recebeu uma resposta errada para a atualização do timestamp")
      print('Timestamp atualizado com sucesso\n')

cmd = ['C01','C02','C03','C04','C06','C07']
# cmd = ['C07']
# print("Comando Enviado {} | type: {} ".format(cmd[0], type(cmd[0])))

key = b'abcdefghijklmnop'

s = serial.Serial(port='COM6',
                  baudrate=115200,
                  bytesize=serial.EIGHTBITS,
                  parity=serial.PARITY_NONE,
                  stopbits=serial.STOPBITS_ONE,
                  timeout=20)




t = ''

for req in cmd:
   
   #- - - Envio da requisição - - -#
   send_cmd = req #Comando
   iv = Random.new().read(AES.block_size) #Chave aleatoria
   cipher = AES.new(key, AES.MODE_CFB, iv)
   crypt_answer = iv + cipher.encrypt(send_cmd.encode()) # Mensagem (iv aleatorio + mensagem criptografada)
   s.write(crypt_answer) # Escrevendo na serial que é conectada ao conversor USB-RS232

   print("Msg enviada: %s | Criptografada: %s" % (send_cmd, crypt_answer))

   #Rotina de atualização do timestamp -> Encerra após a função 
   if req == 'C06':
      timestamp_update()
      continue
      
   
   msg = ''

   try:
      #- - - Análise da resposta - - -#
      ack = s.read(21) # Buffer de resposta iv(16 bytes) + (R0X 3 bytes) + (2 bytes de informação do protocolo)
      iv = ack[:AES.block_size] # Extraindo o iv aleatório
      cipher = AES.new(key, AES.MODE_CFB, iv) 

      msg = cipher.decrypt(ack[AES.block_size:]) # Descriptografia dos 5 bytes recebidos

      # É R07? -> Ler a serial de acordo com o número de logs informado
      if msg[0] == 82 and msg[1] == 48 and msg[2] == 55:
         logs = (msg[3]<<8 | msg[4])
         print(f"Logs Retornados: {logs}")
         ack = s.read( logs * 10)
         msg = cipher.decrypt(ack[:logs*10])
         logs_csv(msg)
      else:  #R01/02/03/04? -> A informação está no primeiro byte de acordo com o protocolo
         print("Resposta: %c%c%c -> " % (msg[0],msg[1],msg[2]),end = ' ')
         print(bin(msg[3]),end='\n\n')

   except Exception:
      print('Nao recebeu')
   
   time.sleep(1)

s.close()

