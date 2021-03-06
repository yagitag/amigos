#!/usr/bin/env python3

import html
import re


def extractInfo(html_page):
  ttsurl_val = getTTSURL(html_page)
  params = {}
  for param in ('asr_langs', 'expire', 'signature', 'v'):
    params[param] = getUrlParamVal(ttsurl_val, param)
  related_videos = [url for url in getRelatedVideos(html_page)]
  return (params, related_videos)



def getUrlParamVal(url, param):
  start = url.index('?') + 1
  s_pat = param + '='
  start = url.index(s_pat, start) + len(s_pat)
  end = url.find('&', start)
  if end == -1: end = len(url)
  return url[start:end]



def search(text, start_pattern, end_pattern, start_position = 0):
  start = text.index(start_pattern, start_position) + len(start_pattern)
  end = text.index(end_pattern, start)
  return (start, end)



def getTTSURL(html):
  start = html.index('"ttsurl"') + len('"ttsurl"')
  (start, end) = search(html, '"', '"', start)
  return html[start:end].replace('\/', '/').replace('\\u0026', '&')



def getTagParamVal(param, tag):
  (start, end) = search(tag, param + '="', '"')
  return tag[start:end]



def getRelatedVideos(html):
  end = 0
  while True:
    start = html.find('video-list-item related-list-item', end)
    if start == -1: break
    (start, end) = search(html, 'href="', '"', start)
    start = html.index('v=', start) + len('v=')
    amp_idx = html.find('&', start, end)
    if amp_idx != -1: end = amp_idx
    yield html[start:end]



def getTags(tag, text):
  end = 0
  while True:
    start = text.find('<' + tag, end)
    if start == -1: return
    end = text.find('/>', start) + len('/>')
    yield text[start:end]



def countSubWords(sub_xml):
  result = 0
  end = 0
  while True:
    try: (start, end) = search(sub_xml, '<text', '>', end)
    except ValueError: break
    start = end + len('>')
    end = sub_xml.index('</text>', start)
    result += len(sub_xml[start:end].split())
  return result


def wrapCDATA(some_data):
  return "<![CDATA[{0}]]>".format(some_data)



def getTitle(text):
  start = text.index('yt watch-title-container')
  (start, end) = search(text, 'title="', '"', start)
  return wrapCDATA(html.unescape(text[start:end]))



def convertIntoText(text):
  text = text.replace('<br />', '\n')
  #text = re.sub('<a.*?href="(.*?)".*?</a>', '\g<1>', text)
  text = re.sub('<a.*?href="(.*?)".*?</a>', '', text)
  text = html.unescape(text)
  return text



def getDescription(text):
  start = text.index('id="eow-description"')
  (start, end) = search(text, '>', '</p>', start)
  result = text[start:end]
  result = convertIntoText(result)
  return wrapCDATA(result)



def getMetaInfo(text, info_map):
  result = {}
  (start, end) = search(text, '<meta itemprop="height" content="', '">')
  quality = text[start:end]
  result['quality'] = info_map['qualities'].setdefault(quality, len(info_map['qualities']))
  (start, end) = search(text, '<meta itemprop="isFamilyFriendly" content="', '">', end)
  result['is_family_friendly'] = text[start:end]
  (start, end) = search(text, '<meta itemprop="interactionCount" content="', '">', end)
  result['views_cnt'] = int(text[start:end])
  (start, end) = search(text, '<meta itemprop="datePublished" content="', '">', end)
  result['date'] = text[start:end]
  (start, end) = search(text, '<meta itemprop="genre" content="', '">', end)
  vtype = html.unescape(text[start:end])
  result['type'] = info_map['video_types'].setdefault(vtype, len(info_map['video_types']))
  return result



def _getLikeByType(text, ltype, start = 0):
  start = text.index('like-button-renderer-' + ltype + '-button-unclicked')
  (start, end) = search(text, '<span class="yt-uix-button-content">', '</span>', start)
  try: result = int(re.sub('[\s,.]', '', text[start:end]))
  except ValueError: result = 0
  return (result, end)



def getLikes(text):
  result = {}
  (result['like'], end_pos) = _getLikeByType(text, 'like')
  (result['dislike'], end_pos)  = _getLikeByType(text, 'dislike')
  return result



def getUserInfo(text):
  result = {}
  start = text.index('<div class="yt-user-info">') + len('<div class="yt-user-info">')
  (start, end) = search(text, '>', '</a>', start)
  result['author'] = wrapCDATA(html.unescape(text[start:end]))
  start = text.find('yt-subscription-button-subscriber-count-branded-horizontal', end)
  if start != -1:
    (start, end) = search(text, '>', '<', start)
    result['subscribers_cnt'] = int(re.sub('[\s,.]', '', text[start:end]))
  else:
    result['subscribers_cnt'] = '0'
  return result


def getLoudness(text):
  try:
    (start, end) = search(text, '"loudness":"', '"')
    return int(float(text[start:end])) + 100
  except ValueError:
    return '' 



def getKeywords(text):
  try:
    (start, end) = search(text, '"keywords":"', '"')
    return wrapCDATA(text[start:end])
  except ValueError:
    return ''


def parseSub(text, info_map, langs = ('ru', 'en')):
  result = {lang: {} for lang in langs}
  for (info, data) in re.findall('((?:\[[^\]]*\]){3})\s*<.*?>\s*<transcript>(.*?)</transcript>', text, re.S):
    lang = info[1:3]
    result[lang]['type'] = info_map['subs_types'][info[5:9]]
    (text, time_info) = ('', '')
    for m in re.finditer('<text start="(.*?)" dur="(.*?)">(.*?)</text>', data, re.S):
      time_info += "{0} {1}\n".format(m.group(1), m.group(2))
      text += m.group(3).replace('\n', ' ').replace('&amp;', '&') + '\n'
    result[lang]['time_info'] = wrapCDATA(time_info)
    if lang == 'ru': text = re.sub('(и )?# 39; (\S)+', '', text)
    result[lang]['text'] = wrapCDATA(html.unescape(text))
  return result
