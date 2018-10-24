import sys
from antlr4 import *
from tokenizer import *

def main():
  input = FileStream("./dataset/2.c")
  lexer = CLexer(input)
  tokens = CommonTokenStream(lexer)
  tokens.fill()
  print([token.text for token in tokens.tokens][:-1])
  # parser = CParser(stream)

  # tree = parser.startRule()
 
if __name__ == '__main__':
  main()