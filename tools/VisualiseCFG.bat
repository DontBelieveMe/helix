@echo off
dot %1 -Tsvg -Nfontname=Consolas -Nshape=box -o %1.svg
start %1.svg 