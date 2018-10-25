import sys
from antlr4 import *
from tokenizer import *
import glob
import itertools
from collections import defaultdict
from os.path import join
from math import log2

def get_all_substrings(s):
  l = len(s)
  sequence_counts = defaultdict(lambda: 0)
  sequence_lengths = []
  for i in range(l):
    for j in range(i, l):
      sequence_counts[' '.join(s[i:j + 1])] += 1
      sequence_lengths.append([s[i:j + 1], len(s[i:j + 1])])
  return sorted(sequence_lengths, key=lambda x: x[1], reverse=True), sequence_counts

def lcs(tokens_record=[]): 
  sequence_lengths, sequence_counts = get_all_substrings(tokens_record)
  with open('test.tsv', 'w') as f:
    f.write('score,tokens,count,sequence\n')
    for record in sequence_lengths:
      count = sequence_counts[' '.join(record[0])]
      if count == 1:
        continue
      score = log2(record[1]) * log2(count)
      f.write(f'{score}\t{record[1]}\t{count}\t{record[0]}\n')

def perform_tokenizing(files=[]):
  tokens_record = []
  for file in files:
    contents = FileStream(file)
    lexer = CLexer(contents)
    token_stream = CommonTokenStream(lexer)
    token_stream.fill() # read till EOF
    tokens = [token.text for token in token_stream.tokens][:-1]
    tokens_record.append(tokens)
  lcs(tokens_record=list(itertools.chain(*tokens_record)))

def consume_through_path():
  path = input('Enter the directory path of source code files: ')
  files = glob.glob(join(path, "*.c"))
  perform_tokenizing(files)

def consume_through_cli(argv):
  if len(argv) > 1:
    argv.pop(0)
    perform_tokenizing(argv)

 
if __name__ == '__main__':
  if len(sys.argv) > 1: consume_through_cli(sys.argv)
  else: consume_through_path()
