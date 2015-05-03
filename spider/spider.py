#!/usr/bin/env python3

import uids_storage
import log_system
import worker

import configparser
import signal
import queue
import zlib
import os

class Spider:
  def __init__(self, config_path):
    signal.signal(signal.SIGINT, self.handleSIGINT)
    signal.signal(signal.SIGTERM, self.handleSIGTERM)
    self.tasks_queue = queue.Queue()
    self.results_queue = queue.Queue()
    self.proxies_queue = queue.Queue()
    #
    config = configparser.ConfigParser(interpolation = configparser.ExtendedInterpolation(), inline_comment_prefixes = ('#'))
    config.read(config_path)
    #
    common_config = config['COMMON']
    if not os.path.exists(common_config['work_dir']):
      os.makedirs(common_config['work_dir'])
    if not os.path.exists(common_config['log_dir']):
      os.makedirs(common_config['log_dir'])
    #
    spider_config = config['SPIDER']
    self.log = log_system.Log(spider_config['log_file'])
    self.workers_cnt = int(spider_config['workers_cnt'])
    self.check_workers_period = int(spider_config['check_workers_period'])
    with open(spider_config['pid_file'], 'w') as output:
      output.write(str(os.getpid()))
    #
    self.uids = uids_storage.UIdsStorage(**config['UID_STORAGE'])
    #
    worker_config = config['WORKER']
    self.worker_dirs = [worker_config['pages_dump'], worker_config['subs_dump']]
    worker.configure(**worker_config)
    self.workers = []
    self.putWorkersInOrder()
    #
    self.tasks_file = config['EXTERNAL_WORKER']['tasks_file']
    self.proxies_file = config['EXTERNAL_WORKER']['proxies_file']
    self.loadTasks()
    self.loadProxies()


  def loadTasks(self):
    with open(self.tasks_file, 'r') as fd:
      tasks = (line.strip() for line in fd)
      self.setTasks(tasks)


  def putWorkersInOrder(self):
    died_cnt = 0
    for i in range(len(self.workers)):
      if died_cnt:
        self.workers[i - died_cnt] = self.workers[i]
      if not self.workers[i].is_alive():
        died_cnt += 1
    if died_cnt:
      self.workers[-died_cnt:] = []
    #
    if len(self.workers) > self.workers_cnt:
      for _ in range(len(self.workers) - self.workers_cnt):
        self.tasks_queue.put('finish')
    else:
      for _ in range(self.workers_cnt - len(self.workers)):
        new_worker = worker.Worker(self.tasks_queue, self.results_queue, self.proxies_queue)
        self.workers.append(new_worker)
        new_worker.start()


  def loadProxies(self):
    with open(self.proxies_file, 'r') as fd:
      for proxy in fd:
        self.proxies_queue.put('http://' + proxy.strip())


  def setTasks(self, tasks):
    for task in tasks:
      if self.uids.add(zlib.crc32(task.encode())):
        self.tasks_queue.put(task)


  def run(self):
    handled_pages_tmp_cnt = 0
    isAlive = True
    while isAlive:
      isAlive = self.handleResult()
      handled_pages_tmp_cnt += 1
      if handled_pages_tmp_cnt == self.check_workers_period:
        self.putWorkersInOrder()
        self.log.write('Hooray!!! I have handled 1000 more pages :) Total count is ' + str(len(self.uids)))
        handled_pages_tmp_cnt = 0


  def handleResult(self):
    hasToContinue = True
    min_words_cnt = 125 * 2 # avg_words_cnt_per_minute * min_video_time
    try:
      data = self.results_queue.get()
      self.log.write('{0}: wc - {1}, rvc - {2}'.format(data['uid'], data['words_cnt'], len(data['new_tasks'])))
      if data['words_cnt'] > min_words_cnt:
        self.uids.commit(zlib.crc32(data['uid'].encode()))
        self.setTasks(data['new_tasks'])
    except TypeError:
      self.log.write('Saving...')
      hasToContinue = False
      self.uids.flushToDisk()
    return hasToContinue


  def handleSIGINT(self, sig_num, frame):
    self.log.write('OK. LOADING NEW PROXIES')
    workers_cnt = self.workers_cnt
    #выгружаем задачи
    tmp_tasks = []
    while not self.tasks_queue.empty():
      tmp_tasks.append(self.tasks_queue.get())
    #убиваем всех рабочих
    self.workers_cnt = 0
    self.putWorkersInOrder()
    for w_thread in self.workers:
      w_thread.join()
    #очищаем и загружаем прокси
    while not self.proxies_queue.empty():
      self.proxies_queue.get()
    self.loadProxies()
    #загружаем задачи
    self.setTasks(tmp_tasks)
    #ротируем логи
    self.log.rotateLogs()
    #ротируем директории рабочих
    worker.createEmptyDirs(*self.worker_dirs)
    #запускаем рабочих
    self.workers_cnt = workers_cnt
    self.putWorkersInOrder()


  def handleSIGTERM(self, sig_num, frame):
    self.log.write('OK. FINISHING')
    #очищаю список задач
    with open(self.tasks_file, 'w') as output:
      while not self.tasks_queue.empty():
        output.write(self.tasks_queue.get() + '\n')
    #убиваем всех рабочих
    workers_cnt = self.workers_cnt
    self.workers_cnt = 0
    self.putWorkersInOrder()
    for w_thread in self.workers:
      w_thread.join()
    self.results_queue.put('finish')



if __name__ == '__main__':
  spider = Spider('config')
  try:
    spider.run()
  except Exception as exception:
    spider.uids.flushToDisk()
    raise exception
