drop table _time;
drop table users;
create table _time (id integer primary key autoincrement,_date date,_time time);
create table users (id integer primary key autoincrement,username varchar(255) unique not NULL,password varchar(255) not null,registertime date not null,logtime_id integer unique, foreign key(logtime_id) references _time(id));
s