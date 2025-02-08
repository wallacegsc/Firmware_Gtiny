
try:
    import serial
    import tqdm
    import struct
    from Crypto.Cipher import AES
    from Crypto import Random
except Exception as erro:
    print(" erro de importacao em : %s" % erro)
    exit(0)


cmd = ['C05']
#cmd = ['C04']
print("Comando Enviado {} | type: {} ".format(cmd[0], type(cmd[0])))

key = b'abcdefghijklmnop'

s = serial.Serial(port='COM28',
                  baudrate=115200,
                  bytesize=serial.EIGHTBITS,
                  parity=serial.PARITY_NONE,
                  stopbits=serial.STOPBITS_ONE,
                  timeout=20)

t = ''
firm_bin_path = r"C:\Users\LSE\Desktop\Hub\Guardiao\app_nn_tiny\app_nn\build\app.bin"
with open(firm_bin_path, "rb") as f:
    read_data = f.read()
    size_firm = len(read_data)
f.close()


iv = Random.new().read(AES.block_size)
cipher = AES.new(key, AES.MODE_CFB, iv)
crypt_answer = iv + cipher.encrypt(b'C05')
s.write(crypt_answer)

try:
   response = s.read(5) 
   
except Exception:
   print('Nao recebeu')
   exit(0)
if response == b'R05PF':
      raise Exception("Falha ao inicializar a nova partição")
elif response!=b'R05PP':
      raise Exception("Falha ao chamar a requisição")



size_firm_char = struct.pack("I", size_firm)
s.write(size_firm_char)

try:
   response = s.read(5) 
   
except Exception:
   print('Nao recebeu')
   exit(0)

if response == b'R05BF':
      raise Exception("Guardião: OTA Begin fail")
elif response!=b'R05BP':
      raise Exception("OTA Begin fail")



size_packet = 1020

for offset in tqdm.tqdm(range(0, size_firm - size_packet, size_packet), desc="Embarcando", unit=" Pacote"):

   checksum = sum(read_data[offset : offset + size_packet])
   s.write(struct.pack("I", checksum))
   s.write(read_data[offset : offset + size_packet])
   try:
      response = s.read(5) 
      # print("response:",response)
   except Exception:
      print('Nao recebeu')
      exit(0)
   if response==b'R05OK':
      continue
   elif response==b'R05TF':
      raise Exception("Gurdião: Timeout")
   elif response==b'R05CF':
      raise Exception("Checksum falhou")
   elif response==b'R05WF':
      raise Exception("OTA write falhou")
   else:
      raise Exception("O Guardião não respondeu de acordo com o protocolo")


if (size_firm%size_packet != 0):
   checksum = sum(read_data[-(size_firm%size_packet):])
   s.write(struct.pack("I", checksum))
   s.write(read_data[-(size_firm%size_packet):])
   try:
      response = s.read(5) 
   except Exception:
      print('Nao recebeu')
      exit(0)
   
   if response!=b'R05OK':
      raise Exception("O Guardião não respondeu")
   
try:
   response = s.read(5) 
except Exception:
   print('Nao recebeu')
   exit(0)
if response==b'R05CP':
   print("Sucesso: Reiniciando o sistema")
elif response==b'R05EF':
   raise Exception("OTA end fail")
elif response==b'R05SF':
   raise Exception("OTA set fail")
   
   


