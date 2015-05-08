#!/usr/bin/env python3


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



def getTitle(text):
  start = text.index('yt watch-title-container')
  (start, end) = search(text, 'title="', '"', start)
  return text[start:end]



def getDescription(text):
  start = text.index('id="eow-description"')
  (start, end) = search(text, '>', '</p>', start)
  return text[start:end]
