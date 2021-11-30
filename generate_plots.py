import matplotlib.pyplot as plt
import pandas as pd

fontSize = 20

fileInnerAccel = open("hw_performance_log.txt", "r")
fileSoftwareOnly = open("sw_performance_log.txt", "r")

SWCycles_SoftwareOnly = []
BUSCycles_SoftwareOnly = []
HWCycles_SoftwareOnly = []
TotalCycles_SoftwareOnly = []

SWCycles_InnerAccel = []
BUSCycles_InnerAccel = []
HWCycles_InnerAccel = []
TotalCycles_InnerAccel = []

groupedStackedBarChartData = []

rows = 0
while True:
    line_InnerAccel = fileInnerAccel.readline().split(", ")
    line_SoftwareOnly = fileSoftwareOnly.readline().split(", ")

    if line_InnerAccel == [''] or line_SoftwareOnly == ['']:
        break
    
    rows += 1
    SWCycles_InnerAccel.append(int(line_InnerAccel[2]))
    BUSCycles_InnerAccel.append(int(line_InnerAccel[3]))
    HWCycles_InnerAccel.append(int(line_InnerAccel[4]))
    TotalCycles_InnerAccel.append(int(line_InnerAccel[2]) + int(line_InnerAccel[3]) + int(line_InnerAccel[4]))
    
    SWCycles_SoftwareOnly.append(int(line_SoftwareOnly[2]))
    BUSCycles_SoftwareOnly.append(int(line_SoftwareOnly[3]))
    HWCycles_SoftwareOnly.append(0)
    TotalCycles_SoftwareOnly.append(int(line_SoftwareOnly[2]) + int(line_SoftwareOnly[3]))

# Total cycles vs. matrix size for SW only and Inner Accel
fig, ax = plt.subplots()
ax.plot(list(range(rows)), TotalCycles_InnerAccel, label="Hardware Accelerated", linewidth=5.0, linestyle='dotted')
ax.plot(list(range(rows)), TotalCycles_SoftwareOnly, label="Pure Software", linewidth=3.0)
plt.xlabel("Matrix Size, N x N", size=fontSize)
plt.ylabel("Total Cycles", size=fontSize)
ax.legend(loc='upper right', fontsize=fontSize, bbox_to_anchor=(1.05, 1.10))
plt.xticks(fontsize=fontSize)
plt.yticks(fontsize=fontSize)
ax.grid(which='both')
ax.yaxis.offsetText.set_fontsize(fontSize)
plt.show()


# Stacked bar chart for SW only and Inner Accel
dfData = []
for i in range(rows):
    dfData.append(['SW Only', i + 3, SWCycles_SoftwareOnly[i], BUSCycles_SoftwareOnly[i], HWCycles_SoftwareOnly[i]])
for i in range(rows):
    dfData.append(['SW/HW', i + 3, SWCycles_InnerAccel[i], BUSCycles_InnerAccel[i], HWCycles_InnerAccel[i]])

df = pd.DataFrame(columns=['Context', 'Parameter', 'SW Cycles', 'Bus Cycles', 'HW Cycles'],
                  data=dfData.copy())
df.set_index(['Context', 'Parameter'], inplace=True)
df0 = df.reorder_levels(['Parameter', 'Context']).sort_index()

colors = plt.cm.tab20.colors

df0 = df0.unstack(level=-1) # unstack the 'Context' column
fig, ax = plt.subplots()
(df0['SW Cycles']+df0['Bus Cycles']+df0['HW Cycles']).plot(kind='bar', color=[colors[1], colors[0]], rot=0, ax=ax, width=0.75, edgecolor='black')
(df0['Bus Cycles']+df0['HW Cycles']).plot(kind='bar', color=[colors[3], colors[2]], rot=0, ax=ax, width=0.75, edgecolor='black')
(df0['HW Cycles']).plot(kind='bar', color=[colors[5], colors[4]], rot=0, ax=ax, width=0.75, edgecolor='black')

legend_labels = [f'{val} ({context})' for val, context in df0.columns]
ax.legend(legend_labels, fontsize=fontSize)

plt.xlabel("Matrix Size, N x N", size=fontSize)
plt.ylabel("Total Cycles", size=fontSize)

plt.xticks(fontsize=fontSize)
plt.yticks(fontsize=fontSize)
ax.grid(which='major')
ax.yaxis.offsetText.set_fontsize(fontSize)

plt.tight_layout()
plt.show()