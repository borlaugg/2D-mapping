import serial
import time
import datetime
import csv
import math

ser = serial.Serial('/dev/rfcomm0', 9600)
starttime = datetime.datetime.now()
while True:
  #  x = input("On or Off\n")
  #  ser.write(bytearray(x,'ascii'))
  #  time.sleep(0.5)
    x_t = 0
    y_t = 0
    while ser.in_waiting:
        dist = 0
        theta = 0
        time= datetime.datetime.now() - starttime
        
        fieldnames = ["dist(t)","x(t)", "y(t)", "theta(t)", "z_value(t)", "time"]

        with open('Data.csv', 'w') as csv_file:
            csv_writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
            csv_writer.writeheader()

        while True:
            result = ser.readline()
            print(result.decode())

            result = result.decode()
            theta = result[: result.index(",")]
            result = result[result.index(",") + 1 :]
            z_value = result[:result.index(",")]
            result = result[result.index(",") + 1 :]
            dist = result[: result.index("\n")]


            dist = float(dist)
            theta = float(theta)
            x_t = x_t + dist*math.cos(theta)
            y_t = y_t + dist*math.sin(theta)
            z_value_t = float(z_value)

            #count = float(result[:result.index(",")])
            #theta = float(result[result.index(",")+1: ])
            #dist = dist + count
            with open('Data.csv', 'a') as csv_file:
                csv_writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
                info ={
                        "dist(t)": dist,
                        "x(t)": 2*3.14*11*x_t/200,
                        "y(t)":2*3.14*11*y_t/200,
                        "theta(t)": theta,
                        "z_value(t)":z_value_t,
                        "time": (datetime.datetime.now() - starttime).total_seconds()
                    }
                csv_writer.writerow(info)
                

             #time.sleep(1)
