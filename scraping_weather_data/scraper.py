import pandas as pd
import urllib.request
import sqlite3
from bs4 import BeautifulSoup

con = sqlite3.connect('dsa.db')
site="https://tempo.cptec.inpe.br/sp/sao-paulo"   
with urllib.request.urlopen(site) as url:
    page = url.read()
soup = BeautifulSoup(page,"html.parser")
dados00 = soup.find_all('div',attrs={'class': 'previsao'})
dados01 = soup.find_all('div',attrs={'class': 'proximos-dias'})

count=0
weekPrediction=[]
df = pd.DataFrame
today = dados00[0].get_text().split()[1]
dayPredictionList=[]
weekPrediction=[]
count=0
df = pd.DataFrame

for data in dados00:
    dataA = data.get_text().split()
    dataB = dataA[1:2]+[dataA[-6]]+[dataA[-7]]+[dataA[-3]]+[max([dataA[3],dataA[5],dataA[7]])]+dataA[-2:]
    dayPredictionList=dataB
    dayPredictionList.append(today)
    dayPredictionList.append(str(count))
    weekPrediction.append(dayPredictionList)
    count+=1
dayPredictionList.clear

for data in dados01:
    dataA = data.get_text().split()
    dataB = dataA[1:2]+[dataA[2]]+[dataA[3]]+[dataA[6]]+[dataA[-3]]+dataA[-2:]
    dayPredictionList=dataB
    dayPredictionList.append(today)
    dayPredictionList.append(str(count))
    weekPrediction.append(dayPredictionList)
    count+=1

stmt = "INSERT INTO WEATHER VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)"
con.executemany(stmt, weekPrediction)
con.commit()

