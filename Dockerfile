FROM postgres:15

ENV POSTGRES_USER=postgres
ENV POSTGRES_PASSWORD=postgres
ENV POSTGRES_DB=kanba

COPY ./database /docker-entrypoint-initdb.d

EXPOSE 5432
