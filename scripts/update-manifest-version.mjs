import fs from 'node:fs';

const version = process.argv[2];

const path = 'custom_components/actron_air_esphome/manifest.json';
const manifest = JSON.parse(fs.readFileSync(path, 'utf8'));
manifest.version = version;
fs.writeFileSync(path, `${JSON.stringify(manifest, null, 2)}\n`);
