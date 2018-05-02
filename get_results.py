#!/usr/bin/env python3

import math
import sys

import urllib.request
from urllib.request import urlopen, Request, HTTPCookieProcessor
from html.parser import HTMLParser
from http.cookiejar import CookieJar
from enum import Enum
from collections import OrderedDict
from operator import itemgetter
from itertools import islice


our_name = "Tarken"


class Table:
    def __init__(self, *args, **kwargs):
        self.keys = []
        self.data = OrderedDict()

class ParserState(Enum):
    OUTSIDE = 1
    THEAD = 2
    TBODY = 3


def isint(string):
    try:
        int(string)
        return True
    except ValueError:
        return False

# converts values in form 1,234,567.00 to int
def sanitize_value(val):
    try:
        val = val.replace(",","")
        f = float(val)
        return int(f)
    except ValueError:
        return None


class CsrfParser(HTMLParser):
    def handle_starttag(self, tag, attrs):
        if tag == "input" and ("name", "_csrf") in attrs:
            self.value = [ value for key,value in attrs if key == "value" ][0]


class MyRunsParser(HTMLParser):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.table = Table()
        self.state = ParserState.OUTSIDE
        self.pos = 0
        self.current_row = OrderedDict()
        self.name = ""


    def handle_starttag(self, tag, attrs):
        if tag == "thead":
            self.state = ParserState.THEAD
        if tag == "tbody":
            self.state = ParserState.TBODY

    def handle_endtag(self, tag):
        if tag == "tbody":
            self.state = ParserState.OUTSIDE
        if self.state == ParserState.TBODY and tag == "tr":
            self.table.data[self.name] = self.current_row
            self.pos = 0;
            self.current_row = OrderedDict()


    def handle_data(self, data):
        if self.state == ParserState.THEAD and not data.isspace():
            val = data.replace(u'\xa0',u' ').split(' ')[0];
            if isint(val):
                self.table.keys.append(int(val))
            else:
                self.table.keys.append(val)

        if self.state == ParserState.TBODY and not data.isspace():
            if isint(self.table.keys[self.pos]):
                val = sanitize_value(data.strip())
            else:
                val = data.strip()

            self.current_row[self.table.keys[self.pos]] = val
            if self.table.keys[self.pos] == "#":
                self.name = int(data.strip())

            self.pos += 1

class TableParser(HTMLParser):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.table = Table()
        self.state = ParserState.OUTSIDE
        self.pos = 0
        self.current_row = OrderedDict()
        self.name = ""


    def handle_starttag(self, tag, attrs):
        if tag == "thead":
            self.state = ParserState.THEAD
        if tag == "tbody":
            self.state = ParserState.TBODY

    def handle_endtag(self, tag):
        if tag == "tbody":
            self.state = ParserState.OUTSIDE
        if self.state == ParserState.TBODY and tag == "tr":
            self.table.data[self.name] = self.current_row
            self.pos = 0;
            self.current_row = OrderedDict()


    def handle_data(self, data):
        if self.state == ParserState.THEAD and not data.isspace():
            val = data.replace(u'\xa0',u' ').split(' ')[0];
            if isint(val):
                self.table.keys.append(int(val))
            else:
                self.table.keys.append(val)

        if self.state == ParserState.TBODY and not data.isspace():
            if isint(self.table.keys[self.pos]):
                val = sanitize_value(data.strip())
            else:
                val = data.strip()

            self.current_row[self.table.keys[self.pos]] = val
            if self.table.keys[self.pos] == "User":
                self.name = data.strip()

            self.pos += 1


def fetch_my_runs(password):
    cj = CookieJar()
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(cj))
    opener.addheaders = [('User-agent', 'Mozilla/5.0')]
    urllib.request.install_opener(opener)

    authentication_url = 'https://www.optil.io/optilion/login?redirect='
    login_url = 'https://www.optil.io/optilion/login'
    data_url = 'https://www.optil.io/optilion/problem/3025/myruns'

    req = Request(login_url);
    resp = urlopen(req)
    resp_text = resp.read().decode("utf-8")

    parser = CsrfParser()
    parser.feed(resp_text)


    payload = {
        'isRegularLogin': 'true',
        'username': our_name,
        'password': password,
        '_csrf': parser.value
    }
    data = urllib.parse.urlencode(payload)
    binary_data = data.encode('UTF-8')

    req = urllib.request.Request(authentication_url, binary_data)
    req.add_header("Referer",login_url)

    resp = urllib.request.urlopen(req)

    req = urllib.request.Request(data_url)
    resp = urllib.request.urlopen(req)

    return resp.read().decode("utf-8")

def fill_best(table):
    first = OrderedDict()
    best = OrderedDict()
    best_count = OrderedDict()


    for i in range(1,101):
        best_count[i] = 0
        best[i] = math.inf

    _, first_row = next(iter(table.data.items()))

    for i in range(1,101):
        first[i] = first_row[i]

    for _, row in table.data.items():
        for i in range(1,101):
            val = row[i]
            if val is not None:
                if val == best[i]:
                    best_count[i] += 1
                elif val < best[i]:
                    best[i] = val
                    best_count[i] = 1

    table.first = first
    table.best = best
    table.best_count = best_count


def calc_loss(our_row, baseline):
    loss = OrderedDict()
    for i in range(1,101):
        loss[i] = (our_row[i] - baseline[i], our_row[i] / baseline[i])
    return loss

def print_instance(table, loss_vs_first, loss_vs_best, inst, width=13, precision=4):
    ours = table.data[our_name][inst]
    first = next(iter(table.data.items()))[1]
    inst_number = 2*inst - 1
    print("instance{inst:03d}.gr: {ours:{width},}  {best:{width},}  {first:{width},} {lvba:{width},} {lvbr:.{precision}f} {lvfa:{width},} {lvfr:.{precision}f}".format(
            inst=inst_number, ours=ours, first=first[inst], best=table.best[inst],
            lvba=loss_vs_best[inst][0], lvbr=loss_vs_best[inst][1],
            lvfa=loss_vs_first[inst][0], lvfr=loss_vs_first[inst][1],
            width=width, precision=precision)
            )


def main(argv=None):
    if argv is None:
        argv = sys.argv
    response = urlopen("https://www.optil.io/optilion/problem/3025/standing")
    response_text = response.read().decode("utf-8")

    parser = TableParser()
    parser.feed(response_text)
    table = parser.table

    fill_best(table)

    first_row = next(iter(table.data.items()))[1]

    loss_vs_first = calc_loss(table.data[our_name], first_row)
    loss_vs_best = calc_loss(table.data[our_name], table.best)

    for i in range(1,101):
        print_instance(table, loss_vs_first, loss_vs_best, i)

    s = sorted(loss_vs_best.items(), key=lambda x: x[1][1], reverse=True)

    top20 = islice(s,20)

    print("\n\nWorst instances:")

    for inst in top20:
        print_instance(table, loss_vs_first, loss_vs_best, inst[0])

    return 0

if __name__ == "__main__":
    sys.exit(main())

