from os import listdir

with open('test.sh','w') as f:
  for e in listdir('./functional_test'):
    if e.endswith('.sy'):
      f.write('echo '+e+'\n')
      f.write('echo '+e+' >> test.log \n')
      f.write('./myparser ./functional_test/'+e+' >> test.log \n')
      # f.write('.\\myparser.exe .\\functional_test\\'+e+'\n')
