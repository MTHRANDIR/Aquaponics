import pandas as pd
import requests
from bs4 import BeautifulSoup
import sqlite3
from datetime import datetime

today_date = datetime.now()
con = sqlite3.connect('dsa.db')
site="https://www.climatempo.com.br/previsao-do-tempo/15-dias/cidade/484/mairipora-sp"

url = requests.get(site)
page = url.text
soup = BeautifulSoup(page,"html.parser")

dados = soup.find_all('div',attrs={'class': 'row no-gutters wrapper-variables-cards'})
dados00 = soup.find_all('div',attrs={'class': 'date-inside-circle'})

count=0
weekPrediction=[]
df = pd.DataFrame
today = dados00[0].get_text().split()[0]
dayPredictionList = []
weekPrediction = []
count=0
df = pd.DataFrame

for data in dados:
    dataA = data.get_text().split()
    dataB = [today_date.strftime("%x")] + [dataA[1]] + [dataA[2]] + [dataA[4]] + [dataA[14]] + [dataA[-5]] + [dataA[-3]]
    dayPredictionList=dataB
    dayPredictionList.append(today)
    dayPredictionList.append(str(count))
    weekPrediction.append(dayPredictionList)
    count+=1

print(weekPrediction)
stmt = "INSERT INTO WEATHER2 VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)"
con.executemany(stmt, weekPrediction)
con.commit()