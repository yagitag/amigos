#!/usr/bin/env python3

import threading
import requests
import socket
import queue
import os


class GetHtmlError(Exception):
  pass


class Worker(threading.Thread):
  def configure(pages_dump, subs_dump, request_timeout, max_tries_to_get_page):
    if not os.path.exists(pages_dump): os.makedirs(pages_dump)
    if not os.path.exists(subs_dump): os.makedirs(subs_dump)
    Worker.pages_dump = pages_dump
    Worker.subs_dump = subs_dump
    Worker.request_timeout = int(request_timeout)
    Worker.max_tries_to_get_page = int(max_tries_to_get_page)

  def __init__(self, task_queue, result_queue, proxies_queue):
    self.task_queue = task_queue
    self.result_queue = result_queue
    self.proxies_queue = proxies_queue
    threading.Thread.__init__(self)

  def run(self):
    watch_url = 'http://www.youtube.com/watch?v={0}'
    while True:
      task = self.task_queue.get()
      proxy = {'http': self.getNextProxy()}
      if task != 'finish':
        try:
          html_page = self.getHtmlText(watch_url.format(task), 'ytplayer', proxy)
          try: (param_for_subs, related_videos) = self.extractInfo(html_page)
          except ValueError: continue
          (subs, words_cnt) = self.getSubs(param_for_subs, proxy)
          result = {'uid': task, 'new_tasks': related_videos, 'words_cnt': words_cnt}
          self.result_queue.put(result)
          self.save(task, html_page, subs)
          print(task + ': success')
        except GetHtmlError:
          self.task_queue.put(task)
          print(task + ': failed')
      else: break

  def getNextProxy(self):
    proxy = self.proxies_queue.get()
    self.proxies_queue.put(proxy)
    return proxy

  def save(self, task, html_page, subs):
    with open(Worker.pages_dump + '/' + task, 'w') as output:
      output.write(html_page)
    with open(Worker.subs_dump + '/' + task, 'w') as output:
      for (lang, sub) in subs.items():
        output.write(lang + '\n')
        output.write(sub)

  def getHtmlText(self, url, check_text, proxy = None):
    result = ''
    if not proxy: proxy = {'http': self.getNextProxy()}
    for proxies_cnt in range(Worker.max_tries_to_get_page):
      #print(url, proxy)
      try:
        result = requests.get(url, proxies = proxy, timeout = Worker.request_timeout).text
        if check_text in result: break
      except requests.ConnectionError: pass #print("CONNECTION_ERROR")
      except (requests.Timeout, socket.timeout): pass #print('TIMEOUT')
      except Exception as exception: print(exception)
      proxy['http'] = self.getNextProxy()
    if not result:
      raise GetHtmlError
    return result

  def extractInfo(self, html_page):
    ttsurl_val = self.getTTSURL(html_page)
    params = {}
    for param in ('asr_langs', 'expire', 'signature', 'v'):
      params[param] = self.getUrlParamVal(ttsurl_val, param)
    related_videos = [url for url in self.getRelatedVideos(html_page)]
    return (params, related_videos)

  def getSubs(self, params, proxy, necessary_langs = ['ru', 'en']): # в necessary_langs языки отсортированы по важности (чем позднее тем важнее)
    #constant
    api_tdt_pfx = 'http://www.youtube.com/api/timedtext?'
    subs_list_sfx = 'asr_langs={0}&asrs=1&caps=asr&expire={1}&fmts=1&hl=&key=yttt1&signature={2}&sparams=asr_langs%2Ccaps%2Cv%2Cexpire&tlangs=1&ts=&type=list&v={3}'
    auto_sub_sfx = 'asr_langs={0}&caps=asr&expire={1}&format=1&hl=&key=yttt1&kind=asr&lang={4}&name=&signature={2}&sparams=asr_langs%2Ccaps%2Cv%2Cexpire&ts=&type=track&v={3}'
    orig_sub_sfx = 'type=track={1}&name={2}&lang={3}&v={0}'
    #
    result = {lang: '' for lang in necessary_langs}
    subs_list_url = api_tdt_pfx + subs_list_sfx.format(params['asr_langs'], params['expire'], params['signature'], params['v'])
    subs_list = self.getHtmlText(subs_list_url, 'transcript_list', proxy)
    downloaded_sub_cnt = 0
    for track in self.getTags('track', subs_list):
      name = self.getTagParamVal('name', track)
      lang = self.getTagParamVal('lang_code', track)
      if lang in necessary_langs:
        if not name:
          sub_url = api_tdt_pfx + auto_sub_sfx.format(params['asr_langs'], params['expire'], params['signature'], params['v'], lang)
        else:
          v_id = self.getTagParamVal('id', track)
          sub_url = api_tdt_pfx + orig_sub_sfx.format(params['v'], v_id, name, lang)
        sub_text = self.getHtmlText(sub_url, 'text start', proxy)
        result[lang] = sub_text
        downloaded_sub_cnt += 1
    if 0 < downloaded_sub_cnt < len(necessary_langs):
      for lang in necessary_langs:
        if not result[lang]:
          sub_text = self.getHtmlText(sub_url + '&tlang=' + lang, 'text start', proxy)
          result[lang] = sub_text
    return (result, self.countSubWords(result['en']))

  def getUrlParamVal(self, url, param):
    start = url.index('?') + 1
    s_pat = param + '='
    start = url.index(s_pat, start) + len(s_pat)
    end = url.find('&', start)
    if end == -1: end = len(url)
    return url[start:end]

  def search(self, text, start_pattern, end_pattern, start_position = 0):
    start = text.index(start_pattern, start_position) + len(start_pattern)
    end = text.index(end_pattern, start)
    return (start, end)

  def getTTSURL(self, html):
    start = html.index('"ttsurl"') + len('"ttsurl"')
    (start, end) = self.search(html, '"', '"', start)
    return html[start:end].replace('\/', '/').replace('\\u0026', '&')

  def getTagParamVal(self, param, tag):
    (start, end) = self.search(tag, param + '="', '"')
    return tag[start:end]

  def getRelatedVideos(self, html):
    end = 0
    while True:
      start = html.find('video-list-item related-list-item', end)
      if start == -1: break
      (start, end) = self.search(html, 'href="', '"', start)
      start = html.index('v=', start) + len('v=')
      amp_idx = html.find('&', start, end)
      if amp_idx != -1: end = amp_idx
      yield html[start:end]

  def getTags(self, tag, text):
    end = 0
    while True:
      start = text.find('<' + tag, end)
      if start == -1: return
      end = text.find('/>', start) + len('/>')
      yield text[start:end]

  def countSubWords(self, sub_xml):
    result = 0
    end = 0
    while True:
      try: (start, end) = self.search(sub_xml, '<text', '>', end)
      except ValueError: break
      start = end + len('>')
      end = sub_xml.index('</text>', start)
      result += len(sub_xml[start:end].split())
    return result


if __name__ == '__main__':
  import configparser
  config = configparser.ConfigParser(interpolation = configparser.ExtendedInterpolation(), inline_comment_prefixes = ('#'))
  config.read('config')

  Worker.configure(**config['WORKER'])
  w = Worker(None, None, None)
  downloaded_tasks = os.listdir(Worker.pages_dump)

  #with open(config['EXTERNAL_WORKER']['tasks_file'], 'r') as fd:
    #for line in fd:
      #task = line.strip()
  for task in downloaded_tasks:
    with open(Worker.pages_dump + '/' + task, 'r') as tmp_fd:
      html_page = tmp_fd.read()
      try: (p, new_tasks) = w.extractInfo(html_page)
      except ValueError: continue
      for new_task in new_tasks:
        if new_task not in downloaded_tasks:
          print(new_task)
