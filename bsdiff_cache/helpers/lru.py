import re
from collections import OrderedDict


class LRUCache(OrderedDict):
    '''不能存储可变类型对象，不能并发访问set()'''

    def __init__(self, capacity):
        self.capacity = capacity
        self.cache = OrderedDict()

    def get(self, key):
        if key in self.cache:
            value = self.cache.pop(key)
            self.cache[key] = value
        else:
            value = None

        return value

    def set(self, key, value):
        if key in self.cache:
            value = self.cache.pop(key)
            self.cache[key] = value
        else:
            if len(self.cache) == self.capacity:
                self.cache.popitem(last=False)  # pop出第一个item
                self.cache[key] = value
            else:
                self.cache[key] = value


def generate_oldpos_list():
    with open("huawei_patch_out_2.txt", "r") as f:
        lines = f.readlines()
        pos_list = []
        for line in lines:
            if line.find('current control block:') != -1:
                pos_list.append(re.split(",| |\)", line)[7])
    return pos_list


block_size = 1
block_num = 64

pos_list = generate_oldpos_list()
cache = LRUCache(block_num)
hit = 0
miss = 0
pointer = 0
for pos in pos_list:
    pointer += int(pos)
    page_no = pointer // (block_size * 1024 * 1024)
    if cache.get(page_no) == None:
        miss += 1
        cache.set(page_no, 1)
    else:
        hit += 1

print("hit:" + str(hit))
print("miss:" + str(miss))
print("hit rate:" + str(hit / (hit + miss)))
print("total memory usage:" + str(block_num*block_size) + "MB")
