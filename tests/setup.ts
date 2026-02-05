import { Pool } from 'pg';
import fs from 'fs';
import path from 'path';

export async function setup() {
  const adminPool = new Pool({
    host: 'localhost',
    port: 5432,
    database: 'postgres',
    user: 'postgres',
    password: 'postgres',
  });

  // Drop and recreate test database
  await adminPool.query('DROP DATABASE IF EXISTS kanba_test');
  await adminPool.query('CREATE DATABASE kanba_test');
  await adminPool.end();

  // Run schema and functions
  const testPool = new Pool({
    host: 'localhost',
    port: 5432,
    database: 'kanba_test',
    user: 'postgres',
    password: 'postgres',
  });

  const schema = fs.readFileSync(path.resolve(__dirname, '../database/schema.sql'), 'utf8');
  const functions = fs.readFileSync(path.resolve(__dirname, '../database/functions.sql'), 'utf8');
  await testPool.query(schema);
  await testPool.query(functions);
  await testPool.end();
}

export async function teardown() {
  const adminPool = new Pool({
    host: 'localhost',
    port: 5432,
    database: 'postgres',
    user: 'postgres',
    password: 'postgres',
  });
  await adminPool.query('DROP DATABASE IF EXISTS kanba_test');
  await adminPool.end();
}
