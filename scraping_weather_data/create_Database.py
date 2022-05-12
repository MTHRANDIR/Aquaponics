import sqlite3
con = sqlite3.connect('dsa.db')

# query = """DROP TABLE WEATHER;"""
# con.execute(query)
# con.commit()

#create table in sqlite
query = """
CREATE TABLE WEATHER
(day VARCHAR(20), 
 tempMin VARCHAR(20),
 tempMax VARCHAR(20),
 UV VARCHAR(20),
 ProbRain VARCHAR(20),
 sundawn VARCHAR(20),
 sunset VARCHAR(20), 
 today VARCHAR(20), 
 forecastDays VARCHAR(20)
);"""

con.execute(query)
con.commit()