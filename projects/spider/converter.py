#!/usr/bin/env python3

import parser
import common

import time
import glob
import zlib
import json
import sys
import os


class Converter:
  def __init__(self, config):
    self.results_path = config['CONVERTER']['results_path']
    self.flush_after = int(config['CONVERTER']['flush_after'])
    self.info_mapping_file = config['CONVERTER']['info_mapping_file']
    self.common_pages_path = config['WORKER']['pages_dump']
    self.common_subs_path = config['WORKER']['subs_dump']
    if not os.path.exists(self.results_path):
      os.makedirs(self.results_path)
    self.loadInfoMapping()
    self.openNewFile(True)


  def saveInfoMapping(self):
    with open(self.info_mapping_file, 'w') as fd:
      json.dump(self.info_map, fd, sort_keys = True, indent = 2)


  def loadInfoMapping(self):
    if os.path.exists(self.info_mapping_file):
      with open(self.info_mapping_file, 'r') as fd:
        self.info_map = json.load(fd)
    else:
      self.info_map = {'qualities': {}, 'video_types': {}, 'subs_types': {'auto': 0, 'tran': 1, 'orig': 2}}


  def openNewFile(self, is_first_time = False):
    self.char_cnt = 0
    if not is_first_time:
      self.char_cnt += self.fd.write('</items>\n')
      self.fd.close()
      raise Exception
    self.fd = open(self.results_path + '/' + str(int(time.time())) + ".xml", 'w')
    self.char_cnt += self.fd.write('<?xml version="1.0" encoding="utf-8" ?>\n')
    self.char_cnt += self.fd.write('<items>\n')


  def run(self):
    dirs_sfx = [pdir[-len('_??-??-????'):] for pdir in glob.glob(self.common_pages_path + '_????-??-??')]
    for sfx in dirs_sfx:
      pages_dir = self.common_pages_path + sfx
      subs_dir = self.common_subs_path + sfx
      uids = os.listdir(pages_dir)
      for uid in uids:
        try:
          info = {'video_id': uid, 'id': zlib.crc32(uid.encode())}
          with open(pages_dir + '/' + uid, 'r') as fd:
            text = fd.read()
          info.update(self.parsePage(text))
          with open(subs_dir + '/' + uid, 'r') as fd:
            text = fd.read()
          info.update(self.parseSub(text))
          self.save(info)
          os.unlink(pages_dir + '/' + uid)
          os.unlink(subs_dir + '/' + uid)
        except (ValueError, KeyError, FileNotFoundError) as e:
           print("There is problem with " + uid + sfx + '. Reason: ' + str(e))


  def parsePage(self, text):
    result = {}
    result['title'] = parser.getTitle(text)
    result['description'] = parser.getDescription(text)
    result['keywords'] = parser.getKeywords(text)
    result['loudness'] = parser.getLoudness(text)
    result.update(parser.getMetaInfo(text, self.info_map))
    result.update(parser.getUserInfo(text))
    result.update(parser.getLikes(text))
    return result


  def writeDictAsXml(self, fd, dictionary, shift = 0):
    char_cnt = 0
    str_shift = ' ' * shift
    for (tag, content) in dictionary.items():
      if isinstance(content, dict):
        char_cnt += fd.write('{0}<{1}>\n'.format(str_shift, tag))
        char_cnt += self.writeDictAsXml(fd, content, shift + 2)
        char_cnt += fd.write('{0}</{1}>\n'.format(str_shift, tag))
      else:
        char_cnt += fd.write('{0}<{1}>{2}</{1}>\n'.format(str_shift, tag, content))
    return char_cnt


  def parseSub(self, text):
    return parser.parseSub(text, self.info_map)


  def save(self, info):
    self.char_cnt += self.fd.write('  <item>\n')
    self.char_cnt += self.writeDictAsXml(self.fd, info, 4)
    self.char_cnt += self.fd.write('  </item>\n')
    if self.char_cnt > self.flush_after:
      self.openNewFile()




if __name__ == '__main__':
  config = common.getConfig()
  c = Converter(config)
  try: c.run()
  finally: c.saveInfoMapping()
