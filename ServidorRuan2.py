import socket
import time
import sys

try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(8)
    sock.bind(("", 5005)) 
except Exception as e:
    print('Erro ao criar o socket')
    sys.exit()

print('Socket criado')

SETPOINT = 25.00          # temperatura desejada
sobtemp = SETPOINT * 1.30;# margem superior de temperatura
subtemp = SETPOINT * 0.70;# margem inferior de temperatura (leds)
AtuaMax = 255.00          # margem superior de envio ao atuador
AtuaMin = 0               # margem inferior de envio ao atuador
fail = 0           # contador de mensagem erradas recebidas

red = 0
green = 0
blue = 0

 
while True:
    try:
        data, addr = sock.recvfrom(256)
        data = float(data)/1000000       #tranforma em float e divide por 10^6 o que recebe so sensor
        erro = SETPOINT - data           #calcula o erro em relacao ao setpoint, usado para atuar o motor
        #print(f"Temp: {data}")          #debug 
        if data > 0:
            #Calculo de atuacao no led rgb
            #(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min     # funcao de adaptacao do valor
            red  = (data - subtemp)*(AtuaMax-AtuaMin)/(sobtemp-subtemp) + AtuaMin # valor de atuacao ao red

            blue  = 0.9*(AtuaMax - red)                                           # azul é contrário do vermelho com desconto de 10% para equilibrio de brilho
            
            if data < SETPOINT:                                                   # valor de atuacao ao green se ele for menor que o setpoint
                green = 0.9*((data - subtemp)*(AtuaMax-AtuaMin)/(SETPOINT-subtemp) + AtuaMin) # 0,9 por ter um ganho maior que as outras cores no setpoint
            
            if data >= SETPOINT:
                green = 0.9*((data - SETPOINT)*(AtuaMin-AtuaMax)/(sobtemp-SETPOINT) + AtuaMax)

            red = int(red)
            blue = int(blue)
            green = int(green)
            
            message = [fail,'r',red,'g',green,'b',blue,'t',data,'e',erro]      # envia uma lista com identificadores para o atuador 
            message = str(message)
            try:
                sock.sendto(message.encode(), ('10.0.0.255', 5004))
                print("")
                print('Mensagem enviada em broadcast na porta 5004')
                print(message)
            except:
                fail = fail + 1
                print('Falha ao enviar MV')
            time.sleep(0.5)
        else: # se recebeu um valor negativo quer dizer que tem algo errado e entao conta o fail
            fail = fail + 1               
            message = [failReceive,'r',red,'g',green,'b',blue,'t',data,'e',erro]      # envia uma lista com identificadores para o atuador 
            message = str(message)
            try: 
                sock.sendto(message.encode(), ('10.0.0.255', 5004))
                print('Mensagem enviada em broadcast na porta 5004 com erro na leitura da temperatura')
                print(message)
       
            except:#falha ao enviar ao atuador
                fail = fail + 1
                print('Falha ao enviar MV e falha na leitura da temperatura')
            time.sleep(0.5)
    except sock.timeout:
        print('Timeout atingido')
        break
    
print('Encerrando operação')
sock.close()



#com o failReceive da pra fazer um failSafe
#envia primeiro o valor do failReceive
#se nao receber um dado
#failReceive ++
#e no início manda esse contador
#la no arduino
#compara o valor anterior com o novo recebido
#se for maior
#nao deixa atuar no motor e no ledrgb
#mas se atingir um certo valor
#liga entra no modo manual
# e resseta o valor de failsafe no arduino
# e aqui resseta quando atingir um valor tbm que deve ser igual ao do arduino









