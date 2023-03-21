import re


def generate_ctrl_block_list():
    with open("huawei_patch_out_4.txt", "r") as f:
        lines = f.readlines()
        cb_list = []
        time_list = []
        time_tmp = []
        for line in lines:
            if line.find('current control block:') != -1:
                cb = []
                for i in range(4, 9, 2):
                    cb.append(re.split(",| |\)|\(", line)[i])
                cb_list.append(cb)
            if line.find('time1') != -1:
                time_tmp.append(re.split("=", line)[1].strip("\n"))
            if line.find('time2') != -1:
                time_tmp.append(re.split("=", line)[1].strip("\n"))
            if line.find('time3') != -1:
                time_tmp.append(re.split("=", line)[1].strip("\n"))
                time_list.append(time_tmp)
                time_tmp = []
    return cb_list, time_list


cb_list, time_list = generate_ctrl_block_list()
print(len(cb_list))
print(len(time_list))
time1_largecnt = 0
time1_large_list = []
time2_largecnt = 0
time2_large_list = []
time3_largecnt = 0
time3_large_list = []
for times in time_list:
    if int(times[0]) >= 100:
        time1_largecnt += 1
        time1_large_list.append(int(times[0]))
    if int(times[1]) >= 100:
        time2_largecnt += 1
        time2_large_list.append(int(times[1]))
    if int(times[2]) >= 100:
        time3_largecnt += 1
        time3_large_list.append(int(times[2]))
time1_large_list.sort(reverse=True)
time2_large_list.sort(reverse=True)
time3_large_list.sort(reverse=True)
print(time1_largecnt)
print(time1_large_list[:10])
print(sum(time1_large_list) / 1000000.0)
print(time2_largecnt)
print(time2_large_list[:10])
print(sum(time2_large_list) / 1000000.0)
print(time3_largecnt)
print(time3_large_list[:10])
print(sum(time3_large_list) / 1000000.0)
