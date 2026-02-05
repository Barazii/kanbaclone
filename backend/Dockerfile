FROM node:20-alpine

WORKDIR /app

COPY package*.json ./
RUN npm install

COPY . .
RUN npm run build

# Don't hardcode PORT - let Railway set it
EXPOSE 3001

CMD ["node", "dist/index.js"]
