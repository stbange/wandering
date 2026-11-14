// Drives the delivered WASM/SDL application through keyboard, pointer and touch input.
// There is no teleport, state mutation, clock acceleration or direct command hook.
import { createRequire } from 'node:module';
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import {existsSync} from 'node:fs';
const require=createRequire(import.meta.url);
const {chromium}=require('playwright');
const url=process.env.WC_URL||'http://127.0.0.1:8088/wandering.html?seed=42';
const out=new URL('../artifacts/',import.meta.url);await fs.mkdir(out,{recursive:true});
const browser=await chromium.launch({executablePath:process.env.CHROMIUM||(existsSync('/usr/bin/chromium')?'/usr/bin/chromium':chromium.executablePath()),headless:true,args:['--no-sandbox','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']});
const page=await browser.newPage({viewport:{width:1280,height:720}});
const errors=[];page.on('pageerror',e=>errors.push(e.message));
let assertions=0;const check=(condition,message)=>{assertions++;assert.ok(condition,message)};
const state=()=>page.evaluate(()=>gameState());
const frames=async n=>page.waitForFunction(n=>gameState().frame>n+1,n,{timeout:10000});
const key=async k=>{const s=await state();await page.keyboard.press(k);await frames(s.frame)};
async function click(label){const s=await state(),b=s.buttons.find(b=>b.label.toLowerCase().includes(label.toLowerCase()));assert.ok(b,`Missing button ${label}`);await page.mouse.click((b.x+b.w*.5)*1280/960,(b.y+b.h*.5)*720/540);await frames(s.frame);}
async function look(yaw,pitch=0){const s=await state();await page.evaluate(({x,y})=>Module.ccall('wc_web_look',null,['number','number'],[x,y]),{x:yaw-s.yaw,y:pitch-s.pitch});await page.waitForTimeout(60);}
async function aim(x,y,z){const s=await state(),dx=x-s.x,dz=z-s.z;await look(Math.atan2(dx,-dz),Math.atan2(y-(s.y+1.62),Math.hypot(dx,dz)));}
async function walk(x,z,tolerance=.22){let s=await state();const indoor=s.indoor;await look(Math.atan2(x-s.x,-(z-s.z)),0);await page.keyboard.down('w');try{await page.waitForFunction(({x,z,tolerance,indoor})=>{let s=gameState();return Math.hypot(s.x-x,s.z-z)<tolerance;},{x,z,tolerance,indoor},{timeout:22000,polling:'raf'});}finally{await page.keyboard.up('w');}await page.waitForTimeout(50);}
async function shot(name){await page.screenshot({path:new URL(name,out).pathname});console.log('Captured',name);}
try {
 await page.goto(url);await page.waitForFunction(()=>window.wanderingReady,undefined,{timeout:30000});await shot('01-title.png');await click('Begin wandering');
 check((await state()).indoor,'Starts in sanctuary');
 const before=await state();await page.keyboard.down('ArrowRight');await page.waitForTimeout(350);await page.keyboard.up('ArrowRight');let s=await state();check(s.yaw>before.yaw+.2,'Arrow look works');check(s.x===before.x&&s.z===before.z,'Arrow look never walks');await look(0);
 await key('F1');check((await state()).help===1,'Help toggles');await key('Escape');const frozen=(await state()).time;await page.waitForTimeout(300);check((await state()).time===frozen,'Menus pause active time');await key('Escape');await key('F5');
 await walk(0,-4.2);await aim(0,1.55,-5.8);check((await state()).hit===1,'Physical helm ray');await key('e');check((await state()).helm===1,'Occupy helm');
 await page.keyboard.down('w');await page.keyboard.down('d');await page.keyboard.down('Shift');await page.waitForTimeout(1300);await page.keyboard.up('w');await page.keyboard.up('d');await page.keyboard.up('Shift');await key('e');s=await state();check(s.helm===0&&s.speed===0,'Release helm brakes');check(Math.hypot(s.cx,s.cz)>20,'Turbo moves the castle');check(Math.abs(s.x)<.01&&Math.abs(s.z+4.2)<.4,'Interior attachment stable');await shot('02-helm.png');
 await key('F9');s=await state();check(Math.hypot(s.cx,s.cz)<.01&&s.panel===0&&!s.helm,'F9 restores parked responsive play');check(s.help===1,'Help visibility survives restore');await key('F1');
 await walk(-2.6,-1);await aim(-4,1,-1);check((await state()).hit===6,'Workshop collision/ray path');await key('e');check((await state()).panel===4,'Workshop opens');
 for(let i=0;i<3;i++)await click('Saw 2 planks');await click('Spin 2 twine');s=await state();check(s.items[5]===6&&s.items[6]===2,'Craft shared supplies');await shot('03-workshop.png');await key('Escape');
 await key('b');await click('Confirm furnished room');s=await state();check(s.rooms===2,'Build the upper hall peacefully');check(s.items[5]===0&&s.items[6]===0,'Room consumes authoritative supplies');await shot('04-construction.png');await key('Escape');
 await walk(2.7,-3);await aim(4,.8,-3);await key('e');check((await state()).panel===6,'Sunroot planter interaction');await click('Plant sunroot');await key('Escape');
 await walk(2.7,0);await aim(4,.8,0);await key('e');check((await state()).panel===6,'Mint planter interaction');await click('Plant meadow mint');await key('Escape');
 await walk(2.5,4);await aim(4,.8,4);await key('e');check((await state()).panel===7,'Bed offers rest and save');const bedTime=(await state()).time;await click('Rest /');check((await state()).time===bedTime,'Rest advances no simulation');await click('Save journey');await key('Escape');
 await walk(0,6);await aim(0,1,8);check((await state()).hit===2,'Gate interaction ray');await key('e');check((await state()).gate===1,'Deploy adaptive stairs');
 await walk(0,18);s=await state();check(!s.indoor&&s.y<.1&&s.health===100,'Walk down stairs without jumping');await aim(0,6.5,0);await shot('05-hearthstrider.png');
 const parked=[s.cx,s.cz];await walk(18,0);await aim(20,1,0);check((await state()).hit===10,'Outdoor resource interaction');await key('e');check((await state()).items[0]===3,'Gather timber outside');await key('e');check((await state()).items[0]===3,'Depletion blocks duplicate harvest');
 await walk(17,22);await aim(17,1,20);await key('e');check((await state()).panel===8,'Exterior merchant interaction');for(let i=0;i<12;i++)await click('Next item');await click('Buy 1');check((await state()).items[12]===1,'Peaceful alloy trade');await key('Escape');
 await walk(0,43);await walk(-30,43);await walk(-30,35);await walk(-33.5,33);await aim(-35,1,33);await key('e');check((await state()).panel===12,'Reachable foundry notes');await shot('06-foundry-notes.png');await key('Escape');
 await walk(-33,30);await aim(-35,1,30);await key('e');check((await state()).items[12]===2,'One-time foundry salvage');await walk(-33,35);await walk(-24.5,35);await walk(-24.5,24.5);await aim(-24,1,23);await key('e');check((await state()).message.includes('Brake engaged'),'Peaceful maintenance brake');
 await walk(-30,22.5);await aim(-30,1,20);await key('e');s=await state();check(s.items[12]===6&&s.items[5]===8,'Peaceful reserve equals combat reward');check(s.health===100,'Peaceful route avoids damage');await shot('07-foundry.png');
 await walk(-24.5,24.5);await walk(-24.5,35);await walk(-30,35);await walk(-30,43);await walk(0,43);await walk(0,18);await walk(0,6);s=await state();check(s.indoor&&s.health===100,'Walk back up deployed stairs into sanctuary');check(s.cx===parked[0]&&s.cz===parked[1],'Parked castle position stays exact');
 await walk(-2.5,4);await aim(-4,1,4);await key('e');check((await state()).panel===5,'Pantry opens after homecoming');for(let i=0;i<3;i++)await click('Put 1');await key('Escape');
 await walk(2.7,-3);await aim(4,.8,-3);await key('e');await click('Harvest:');s=await state();check(s.items[7]===3,'Active-time garden harvest');check(!s.buttons.some(b=>b.label.includes('Harvest:')),'Harvest action removed after claim');check((await state()).items[7]===3,'No duplicate crop yields');await key('Escape');
 await key('F5');const saved=await state();await key('i');await key('F9');s=await state();check(s.items[12]===saved.items[12]&&s.rooms===2,'Save restores complete inventory and room graph');await shot('08-homecoming.png');
 check(errors.length===0,`No browser runtime errors: ${errors.join(',')}`);
 await fs.writeFile(new URL('game-test.json',out),JSON.stringify({assertions,errors,final:s},null,2));console.log(`${assertions} browser gameplay assertions passed.`);
}catch(e){console.error(e);console.error(await state().catch(()=>({})));await shot('failure.png');process.exitCode=1;}finally{await browser.close();}
