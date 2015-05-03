#!/usr/bin/env python3

import log_system
import common

import urllib.request
import signal
import os



def tee(text, log):
  print(text)
  log.write(text)


def downloadHideMeProxies(fd, secret, text_type = 'plain'):
  #text_type plain, js, xml
  url = 'http://hideme.ru/api/proxylist.php?type=h&code={0}&out={1}&lang=ru'
  response = urllib.request.urlopen(url.format(secret, text_type))
  fd.write(response.read())



def noticeSpider(spider_pid):
  os.kill(spider_pid, signal.SIGINT)



if __name__ == '__main__':
  config = common.getConfig()
  #
  common_config = config['COMMON']
  if not os.path.exists(common_config['work_dir']):
    os.makedirs(common_config['work_dir'])
  if not os.path.exists(common_config['log_dir']):
    os.makedirs(common_config['log_dir'])
  #
  log_file = config['EXTERNAL_WORKER']['log_file']
  tasks_file = config['EXTERNAL_WORKER']['tasks_file']
  if not os.path.exists(log_file): open(log_file, 'w').close()
  if not os.path.exists(tasks_file): open(tasks_file, 'w').close()
  log = log_system.Log(log_file)
  #
  tee('Update proxies', log)
  hide_me_secret = open(config['EXTERNAL_WORKER']['hide_me_secret_file'], 'r').read().strip()
  proxies_file = config['EXTERNAL_WORKER']['proxies_file']
  with open(proxies_file, 'wb') as proxies_fd:
    downloadHideMeProxies(proxies_fd, hide_me_secret)
  #
  tee('Notice spider', log)
  spider_pid_file = config ['SPIDER']['pid_file']
  if os.path.exists(spider_pid_file):
    with open(spider_pid_file) as spid_fd: spider_pid = int(spid_fd.read())
    noticeSpider(spider_pid)

