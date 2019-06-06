import serial
import simplekml

import matplotlib.pyplot as plt

kml_file_name = "position.kml"
output_file_name = "output.csv"

ser = serial.Serial('COM6')
coordinates = []
alt = []
time = []

#create/overwrite output.csv
with open(output_file_name, 'w') as output:
    output.write("time lat lon alt temp hum whatever\n")

#kml file settings
skml = simplekml.Kml()
ls = skml.newlinestring(name='FlightPath')
ls.extrude = 1
ls.altitudemode = simplekml.AltitudeMode.relativetoground
ls.style.linestyle.width = 5
ls.style.linestyle.width = 5
ls.style.linestyle.color = '7f00ffff'
ls.style.polystyle.color = '7f00ff00'

#clear buffer and ongoing transmition
ser.flush()
flush = ser.readline()

#pyplot (temporary)
plt.ion()
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)

while True:
    line = str(ser.readline(), "utf-8")
    data = line[0:-1].split(" ")

    new_point = (float(data[1]), float(data[2]), float(data[3]))

    coordinates.append(new_point)
    time.append(float(data[0]))
    alt.append(float(data[3]))

    print(data)

    ls.coords = coordinates
    skml.save(kml_file_name)

    with open(output_file_name, 'a') as output:
        output.write(line)

    # pyplot (temporary)
    ax.clear()
    ax.plot(time, alt)
    plt.show()
    plt.pause(0.01)

