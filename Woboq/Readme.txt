To generate static web pages of all the Tacent library code:
1) Compile in WSL the forked woboq_codebrowser. Build both codebrowser_generator
   and codebrowser_indexgenerator
2) Copy these over to tacent/Woboq
3) Edit WoboqTacent.cfg
4) Run in WSL terminal gen_tacent_html
5) Copy what you see in www/Tacent into docs/codebrowser
6) Copy what you see in www/data into docs/data
