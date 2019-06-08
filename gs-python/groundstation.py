#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import serial_mock
import simplekml
import matplotlib.pyplot as plt
from drawnow import *

import datetime

# konfiguracja
kml_file_name = "position.kml"
output_file_name = "output.csv"

coordinates = []
alt = []
all_pressure = []
time = []


def drawFigure(): # Create a function that makes our desired plot

    plt.title('Live data') # Plot the title
    plt.grid(True) # Turn the grid on
    plt.ylabel('Altitude [m]') # Set ylabels

    plt.plot(time, alt, 'ro-', label='Altitude [m]') # plot the altitude

    plt2=plt.twinx()
    plt2.set_ylabel('Pressure [Pa]') # Set ylabels

    plt2.plot(time, all_pressure, 'bo-', label='Pressure [Pa]') # plot the altitude

    plt.legend(loc='upper left') # plot the legend

#ser = serial.Serial('COM6')
ser = serial_mock.SerialMock('..\\mock\\gs-data-feeder\\test_data.csv', 0.1)

# create/append output.csv + header
with open(output_file_name, 'a') as output:
    output.write("gs_timestamp;timestamp;received_packets_qty;rssi;satellites;longitude;latitude;altitude;temperature;pressure;humidity\n")

# te ustawienia dostarczyć jako gotową paczkę i tylko je wyjasnić
# kml file settings
skml = simplekml.Kml()
ls = skml.newlinestring(name='FlightPath')
ls.extrude = 1
ls.altitudemode = simplekml.AltitudeMode.relativetoground
ls.style.linestyle.width = 5
ls.style.linestyle.color = '7f00ffff'
ls.style.polystyle.color = '7f00ff00'

# clear buffer and ongoing transmition
ser.flush()
ser.readline()

# pyplot (temporary)
plt.ion()

while True:

    # odczytaj jedną linię danych z Serial portu
    line = ser.readline()
    print(line)

    # weź datę i godzinę odczytania danych
    gs_timestamp = str(datetime.datetime.now())

    # wyświetl dla użytkownika dane w terminalu - to zachowamy w finalnym programie, reszte usuniemy
    line_with_timestamp = gs_timestamp + ";" + line
    print(line_with_timestamp)

    # zapisz linię do pliku tekstowego w celu backupu
    with open(output_file_name, 'a') as output:
        output.write(line_with_timestamp)


    # usuń znaki nowej linii z końca `strip`
    stripped_line = line.strip()
    print(stripped_line)

    # podziel linię na poszczególne pola danych
    data = stripped_line.split(";")
    print(data)

    # przekonwertuj wartości
    timestamp =             float(data[0])/1000.0 # konwersja z milisekund na sekundy
    received_packets_qty =  int(data[1])
    rssi =                  int(data[2])
    satellites =            int(data[3])
    longitude =             float(data[4])
    latitude =              float(data[5])
    altitude =              float(data[6])
    temperature =           int(data[7])/10.0 # konwersja decy stopnie Celsjusza na stopnie Celsjusza
    pressure =              float(data[8])
    humidity =              int(data[9])


    # dodaj koordynaty GPS do pliku KLM dla Google Earth
    new_coordinates_point = (longitude, latitude, altitude)
    coordinates.append(new_coordinates_point)
    ls.coords = coordinates
    skml.save(kml_file_name)


    # dodaj aktualną wysokość i czas do tablicy z danymi
    time.append(timestamp)
    alt.append(altitude)
    all_pressure.append(pressure)

    # pyplot (temporary)
    drawnow(drawFigure)
