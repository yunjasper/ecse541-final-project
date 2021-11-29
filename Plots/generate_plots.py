import matplotlib.pyplot as plt
import pandas as pd

fontSize = 20

fileInnerAccel = open("hw_performance_log.txt", "r")
fileSoftwareOnly = open("sw_performance_log.txt", "r")

HWCycles_InnerAccel = []
SWCycles_InnerAccel = []
TotalCycles_InnerAccel = []

HWCycles_SoftwareOnly = []
SWCycles_SoftwareOnly = []
TotalCycles_SoftwareOnly = []

groupedStackedBarChartData = []

rows = 0
while True:
    line_InnerAccel = fileInnerAccel.readline().split(", ")
    line_SoftwareOnly = fileSoftwareOnly.readline().split(", ")

    if line_InnerAccel == [''] or line_SoftwareOnly == ['']:
        break
    
    rows += 1
    HWCycles_InnerAccel.append(int(line_InnerAccel[3]))
    SWCycles_InnerAccel.append(int(line_InnerAccel[2]))
    TotalCycles_InnerAccel.append(int(line_InnerAccel[2]) + int(line_InnerAccel[3]))
    
    HWCycles_SoftwareOnly.append(0)
    SWCycles_SoftwareOnly.append(int(line_SoftwareOnly[2]))
    TotalCycles_SoftwareOnly.append(int(line_SoftwareOnly[2]))


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
    dfData.append(['SW Only', i + 3, SWCycles_SoftwareOnly[i], HWCycles_SoftwareOnly[i]])
for i in range(rows):
    dfData.append(['SW/HW', i + 3, SWCycles_InnerAccel[i], HWCycles_InnerAccel[i]])

for row in dfData:
    print(row)

df = pd.DataFrame(columns=['Context', 'Parameter', 'SW Cycles', 'HW Cycles'],
                  data=dfData.copy())
df.set_index(['Context', 'Parameter'], inplace=True)
df0 = df.reorder_levels(['Parameter', 'Context']).sort_index()

colors = plt.cm.Paired.colors

df0 = df0.unstack(level=-1) # unstack the 'Context' column
fig, ax = plt.subplots()
(df0['SW Cycles']+df0['HW Cycles']).plot(kind='bar', color=[colors[1], colors[0]], rot=0, ax=ax, width=0.66)
(df0['HW Cycles']).plot(kind='bar', color=[colors[3], colors[2]], rot=0, ax=ax, width=0.66)

legend_labels = [f'{val} ({context})' for val, context in df0.columns]
legend_labels.remove('HW Cycles (SW Only)')
ax.legend(legend_labels)
ax.legend(loc='upper left', fontsize=fontSize)

plt.xlabel("Matrix Size, N x N", size=fontSize)
plt.ylabel("Total Cycles", size=fontSize)

plt.xticks(fontsize=fontSize)
plt.yticks(fontsize=fontSize)
ax.grid(which='both')
ax.yaxis.offsetText.set_fontsize(fontSize)

plt.tight_layout()
plt.show()