import sqlite3
import time
import Adafruit_BBIO.GPIO as GPIO
import Adafruit_BBIO.UART as UART
import serial
# import Adafruit_DHT as dht
from datetime import datetime

# Initial the dht device, with data pin connected to:

UART.setup("UART1")  # RX - P9_26, TX - P9_24
# learn.adafruit.com/setting-up-io-python-library-on-beaglebone-black/uart
GPIO.setup("P8_11", GPIO.IN)
# dhtDevice = dht.DHT11
ser = serial.Serial(port="/dev/ttyO1", baudrate=9600)  # <- 9600 na arduino też
ser.close()
ser.open()

cols = ["P8_7", "P8_8", "P8_9", "P8_10"]
rows = ["P8_15", "P8_16", "P8_17", "P8_18"]
keys = (
    ("1", "2", "3", "A"),
    ("4", "5", "6", "B"),
    ("7", "8", "9", "C"),
    ("*", "0", "#", "D")
)


def klawiatura():
    while True:
        for i, row in enumerate(rows):
            GPIO.output(row, GPIO.HIGH)
            for j, col in enumerate(cols):
                if GPIO.input(col):
                    return keys[i][j]
            GPIO.output(row, GPIO.LOW)


def refresh_log_view(cursor):
    cursor.execute("""SELECT * FROM log""")
    for row in cursor.fetchall():
        print(row[0] + '  ' + row[1])


for i in rows:
    GPIO.setup(i, GPIO.OUT)
for i in cols:
    GPIO.setup(i, GPIO.IN)

print("Proba otworzenia polaczenia z Arduino")
if ser.isOpen():
    print("Polaczenie otwarte")
    try:
        # Creates or opens a file called mydb with a SQLite3 DB
        db = sqlite3.connect('anti-burglary')
        # Get a cursor object
        cursor = db.cursor()
        # Check if table users does not exist and create it
        cursor.execute("""CREATE TABLE IF NOT EXISTS
                              log(date STRING PRIMARY KEY, event STRING)""")
        # Commit the change
        #Initial pin
        pin = ''
        print("Otwarto polaczenie z baza danych.")
        while True:
            print("Oczekuje na nadanie informacji o zdarzeniu od Arduino.")
            line = ser.readline()
            line = line.decode("ascii")
            print(line)
            #cursor.execute("""INSERT INTO log VALUES(%s, %s)""" % (str(datetime.now(), line)))
            #refresh_log_view(cursor)
            if line == 'ALARM_TRIGGERED':
                pin = ''
            while True:
                prev_pin = pin
                pin = pin + klawiatura()
                time.sleep(0.5)
                if(prev_pin != pin):
                    print(pin)
                # tu można podświetlanie tych diód?
                if pin == '1234':
                    ser.write('correct\n'.encode('ascii'))
                if len(pin) == 4:
                    pin = ''
                    break
                else:
                    pass
                    # dioda?

        db.commit()
        # Catch the exception
    except Exception as e:
        # Roll back any change if something goes wrong
        print("Napotkano problem z baza danych.")
        db.rollback()
        raise e
    finally:
    # Close the db connection
        print("Zamykam baze danych.")
        db.close()
else:
    print("Nie udalo sie otworzyc polaczenia szeregowego")
ser.close()
UART.cleanup()
