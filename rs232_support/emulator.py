
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


cmd = ['C07']
#cmd = ['C04']
# print("Comando Enviado {} | type: {} ".format(cmd[0], type(cmd[0])))

key = b'abcdefghijklmnop'

s = serial.Serial(port='COM7',
                  baudrate=115200,
                  bytesize=serial.EIGHTBITS,
                  parity=serial.PARITY_NONE,
                  stopbits=serial.STOPBITS_ONE,
                  timeout=20)

t = ''

for req in cmd:
   send_cmd = req
   iv = Random.new().read(AES.block_size)
   cipher = AES.new(key, AES.MODE_CFB, iv)
   crypt_answer = iv + cipher.encrypt(send_cmd.encode())
   s.write(crypt_answer)

   if req == 'C06':
      try:
         response = s.read(2) 
         # print("response:",response)
      except Exception:
         print('Nao recebeu o OK para o timestamp')
         exit(0)
      if response!=b'OK':
         raise Exception("Recebeu uma resposta errada para a preparação do timestamp")
      print("Msg enviada: %s | Criptografada: %s" % (send_cmd, crypt_answer))
      data_timestamp = int(time.time()) # byte1 byte2 byte3 byte4  (mais -> menos) significativo
      byte1 = chr((data_timestamp >> 24) & 0xFF)
      byte2 = chr((data_timestamp >> 16) & 0xFF)
      byte3 = chr((data_timestamp >> 8) & 0xFF)
      byte4 = chr(data_timestamp & 0xFF)
      data_timestamp = byte4 + byte3 + byte2 + byte1
      
      iv = Random.new().read(AES.block_size)
      cipher = AES.new(key, AES.MODE_CFB, iv)
      crypt_answer = iv + cipher.encrypt(data_timestamp.encode('latin-1'))
      s.write(crypt_answer)
      
      try:
         response = s.read(3) 
         # print("response:",response)
      except Exception:
         print('Atualização do timestamp falhou')
         exit(0)
      if response!=b'TOK':
         raise Exception("Recebeu uma resposta errada para a atualização do timestamp")
      print('Timestamp atualizado\n')
      continue
      
   print("Msg enviada: %s | Criptografada: %s" % (send_cmd, crypt_answer))
   msg = ''

   try:
      
      ack = s.read(21) 
      iv = ack[:AES.block_size]
      cipher = AES.new(key, AES.MODE_CFB, iv) 

      msg = cipher.decrypt(ack[AES.block_size:])

      if msg[0] == 82 and msg[1] == 48 and msg[2] == 55:
         logs = (msg[3]<<8 | msg[4])
         print(f"Logs Retornados: {logs}\n")
         ack = s.read( logs * 10)
         msg = cipher.decrypt(ack[:logs*10])
      else:
         print("Resposta: %c%c%c -> " % (msg[0],msg[1],msg[2]),end = ' ')
         print(bin(msg[3]),end='\n\n')


      # tam = struct.unpack("H", ack[2:4])[0]
      # ack = s.read(tam)
      # print("len: ", len(ack))
      # ack = ack[:size]
      if req == 'C07':
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
         df.to_csv("log_info.csv", index=False)
         print("Escreveu csv")



      # if cmd[0] == 'C07':
      #    for n in range(0,10):
      #       print(f"{str(msg[n]).center(20)}{str(msg[n+10]).center(20)}{str(msg[n+20]).center(20)}{str(msg[n+30]).center(20)}{str(msg[n+40]).center(20)}")

      #    print('\n')
      #    for n in range(0,41,10):
      #       if msg[n] == 160 or msg[n] == 172:
      #          print(f"{' Ataque'.center(20)}",end='')
      #       elif msg[n] == 163 or msg[n] == 175:
      #          print(f"{' Event'.center(20)}",end='')

      #    print()
      #    for n in range(0,41,10):
      #       if msg[n+5] == 192 or msg[n+5] == 193:
      #          print(f"{'Case'.center(20)}",end='')
      #          continue
            
      #       atack_type = msg[n+5] & 0x07
      #       if atack_type == 1:
      #          print(f"{'Normal'.center(20)}",end='')
      #       if atack_type == 2:
      #          print(f"{'Térmico'.center(20)}",end='')
      #       if atack_type == 4:
      #          print(f"{'Vibração'.center(20)}",end='')  

      #    print()
      #    for n in range(0,41,10):
      #       data_timestamp = 0
      #       data_timestamp = msg[n+4] << 24 | msg[n+3] << 16 | msg[n+2] << 8 | msg[n+1]
      #       if data_timestamp == 0:
      #          print(f"{'0'.center(20)}",end = '')
      #          continue
      #       data_timestamp = datetime.datetime.fromtimestamp(data_timestamp).strftime('%H:%M:%S')
      #       print(f"{data_timestamp.center(20)}",end='')

      #    print()
      #    for n in range(0,41,10):
      #       data_timestamp = 0
      #       data_timestamp = msg[n+4] << 24 | msg[n+3] << 16 | msg[n+2] << 8 | msg[n+1]
      #       if data_timestamp == 0:
      #          print(f"{'0'.center(20)}",end='')
      #          continue
      #       data_timestamp = datetime.datetime.fromtimestamp(data_timestamp).strftime('%d-%m-%Y')
      #       print(f"{data_timestamp.center(20)}",end='')

      #    print()
             


      
      # print("\nDados:\n")

      
      # print("size: ",len(msg))
      # num = 0
      # for item in msg:
      #    print(item)

      # print((msg[0]) == 82 )
      # print((msg[1]) ) 
      # print( (msg[2]) )
      # print("%d"% (msg[3]<<8 | msg[4]))
      

      # second_byte = bin(msg[3])
      # third_byte = bin(msg[4])
      # print(msg[0])
      # print(msg[1])
      # print(msg[2])
      # print(bin(msg[3]))
      # print(msg[4])

      # print('Msg recebida: %s' % msg)
      # print("Resposta DMI: %s | Decriptografada %s " % (ack, msg))
      #print("--binarios--")
      #print(second_byte)
      #print(third_byte)
      #print("------------")

   except Exception:
      print('Nao recebeu')
   
   time.sleep(1)

s.close()

