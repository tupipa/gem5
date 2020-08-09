

import os
import os.path
import sys

def ap_space(str):
    return str + ' '

class PhysicalTraceRecord:
  def __init__(self):
      self.vadd = None

  def init_with_values(self, vaddr, host_vaddr, \
               paddr, rw, size, instcolor = 0, value = 0):
    self.vaddr = vaddr
    self.host_vaddr = host_vaddr
    self.paddr = paddr
    self.rw = rw
    self.size = size
    self.instcolor = instcolor
    self.value = value

  def init_with_str_record(self, str):
    str = str.strip()
    values = str.split()
    self.rw = values[0]
    self.vaddr = int(values[1], 16)
    self.host_vaddr = int(values[2], 16)
    self.paddr = int(values[3], 16)
    self.size = int(values[4], 16)
    self.instcolor = int(values[5], 16)
    self.value = int(values[6], 16)
    return True

  def __str__(self):
    retstr = ''
    retstr += ap_space(self.rw)
    retstr += ap_space(hex(self.vaddr))
    retstr += ap_space(hex(self.host_vaddr))
    retstr += ap_space(hex(self.paddr))
    retstr += ap_space(hex(self.size))
    retstr += ap_space(hex(self.instcolor))
    retstr += ap_space(hex(self.value))
    retstr += '\n'

    return retstr


