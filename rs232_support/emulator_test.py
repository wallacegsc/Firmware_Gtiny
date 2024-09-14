
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

s = serial.Serial(port='COM7',
                  baudrate=115200,
                  bytesize=serial.EIGHTBITS,
                  parity=serial.PARITY_NONE,
                  stopbits=serial.STOPBITS_ONE,
                  timeout=20)

t = ''
firm_bin_path = r"C:\Users\LSE\Desktop\Hub\Guardiao\app_nn\app_nn\build\app.bin"
with open(firm_bin_path, "rb") as f:
    read_data = f.read()
    size_firm = len(read_data)
f.close()


iv = Random.new().read(AES.block_size)
cipher = AES.new(key, AES.MODE_CFB, iv)
crypt_answer = iv + cipher.encrypt(b'C05')
s.write(crypt_answer)

try:
   response = s.read(2) 
   
except Exception:
   print('Nao recebeu')
   exit(0)
if response!=b'OK':
      raise Exception("Falha ao chamar a requisição")



size_firm_char = struct.pack("I", size_firm)
s.write(size_firm_char)

try:
   response = s.read(3) 
   
except Exception:
   print('Nao recebeu')
   exit(0)
if response!=b'OBP':
      raise Exception("OTA Begin fail")



size_packet = 1020

for offset in tqdm.tqdm(range(0, size_firm - size_packet, size_packet), desc="Embarcando", unit=" Pacote"):
   # print(offset)
   checksum = sum(read_data[offset : offset + size_packet])
   s.write(struct.pack("I", checksum))
   s.write(read_data[offset : offset + size_packet])
   try:
      response = s.read(3) 
      # print("response:",response)
   except Exception:
      print('Nao recebeu')
      exit(0)

   if response==b'CSF':
      raise Exception("Checksum falhou")
   if response!=b'R05':
      raise Exception("O Guardião não respondeu")


if (size_firm%size_packet != 0):
   checksum = sum(read_data[-(size_firm%size_packet):])
   s.write(struct.pack("I", checksum))
   s.write(read_data[-(size_firm%size_packet):])
   try:
      response = s.read(3) 
   except Exception:
      print('Nao recebeu')
      exit(0)
   
   if response!=b'R05':
      raise Exception("O Guardião não respondeu")

