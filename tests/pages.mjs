// Exercise the staged/deployed Pages routes, including a project URL prefix.
import {createRequire} from 'node:module';
import {existsSync} from 'node:fs';
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';
const require=createRequire(import.meta.url),{chromium}=require('playwright');
const base=(process.env.WC_PAGES_URL||'http://127.0.0.1:8091/wandering/').replace(/\/?$/,'/');
const browser=await chromium.launch({executablePath:process.env.CHROMIUM||(existsSync('/usr/bin/chromium')?'/usr/bin/chromium':chromium.executablePath()),headless:true,args:['--no-sandbox','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']});
const out=new URL('../artifacts/',import.meta.url);await fs.mkdir(out,{recursive:true});
const errors=[];let checks=0;
function check(condition,message){checks++;assert.ok(condition,message);}
async function ready(page,url){
 page.on('pageerror',e=>errors.push(e.message));
 page.on('response',r=>{if(r.status()>=400)errors.push(`${r.status()} ${r.url()}`);});
 const response=await page.goto(url);check(response.status()===200,`Entry responds at ${url}`);
 await page.waitForFunction(()=>window.wanderingReady,undefined,{timeout:45000});
 await page.waitForTimeout(100);
}
try{
 for(const [edition,path] of [['web',''],['webxr','webxr/']]){
  const context=await browser.newContext({viewport:{width:1280,height:720}});
  const page=await context.newPage();await ready(page,base+path+'?seed=42');
  check(await page.evaluate(()=>window.wanderingEdition)===edition,'Distinct edition identity');
  const title=await page.title();check(title.endsWith(edition==='web'?'— Web':'— WebXR'),'Distinct page title');
  const runtime=await page.evaluate(()=>({bridge:typeof window.WanderingXR,entry:typeof Module._wc_xr_view,seed:gameState().seed}));
  check(runtime.seed===42,'Seed query reaches the game under a project prefix');
  check(runtime.bridge===(edition==='web'?'undefined':'object'),'XR bridge included only in immersive build');
  check(runtime.entry===(edition==='web'?'undefined':'function'),'Separate compiled WASM exports');
  if(edition==='web')check(await page.locator('#xr').isHidden(),'No headset action in standard edition');
  else{
   check(await page.locator('#xr').isVisible(),'Dedicated XR entry is visible');
   await page.waitForFunction(()=>document.getElementById('xr').textContent!=='Checking WebXR…');
   check((await page.locator('#xr').textContent())==='Enter WebXR'||await page.locator('#xr-status').isVisible(),'XR availability is explained');
  }
  await page.keyboard.press('Enter');await page.waitForFunction(()=>gameState().panel===0);
  const start=await page.evaluate(()=>gameState());await page.keyboard.down('w');await page.waitForTimeout(300);await page.keyboard.up('w');
  check((await page.evaluate(()=>gameState())).z<start.z-.3,'Each page is a playable first-person game');
  await page.keyboard.down('w');await page.waitForFunction(()=>gameState().z< -3.8);await page.keyboard.up('w');
  let frame=await page.evaluate(()=>{const s=gameState();Module.ccall('wc_web_look',null,['number','number'],[Math.PI-s.yaw,-s.pitch]);return s.frame;});
  await page.waitForFunction(f=>gameState().frame>f+1,frame);check(await page.evaluate(()=>gameState().hit)!==1,'Look away from the helm before the interaction regression');
  await page.evaluate(()=>{const s=gameState(),yaw=Math.atan2(-s.x,5.8+s.z),pitch=Math.atan2(1.55-s.y-1.62,Math.hypot(s.x,5.8+s.z));Module.ccall('wc_web_look',null,['number','number'],[yaw-s.yaw,pitch-s.pitch]);gameAction(-1);});
  check(await page.evaluate(()=>gameState().helm)===1,'Look and interact in the same event uses the current ray');
  await page.keyboard.press('Escape');await page.waitForFunction(()=>gameState().panel===2);
  const time=await page.evaluate(()=>gameState().time);await page.waitForTimeout(150);
  check((await page.evaluate(()=>gameState())).time===time,'Pause still works after staging');
  const wasm=await context.request.get(base+path+'wandering.wasm');check(wasm.ok()&&wasm.headers()['content-type']?.includes('application/wasm'),'WASM is served with the correct MIME type');
  await page.screenshot({path:new URL(`pages-${edition}.png`,out).pathname});
  const link=page.locator('#edition-link');check(await link.isVisible(),'Each published page links to the other');
  const target=new URL(edition==='web'?'webxr/':'',base).href;check(await link.evaluate(a=>a.href)===target,'Edition link preserves the project prefix');
  await link.click();await page.waitForFunction(()=>window.wanderingReady);check(page.url()===target,'Edition link opens its own page');
  await context.close();
 }
 const context=await browser.newContext({viewport:{width:844,height:390},isMobile:true,hasTouch:true});
 const mobile=await context.newPage();await ready(mobile,base+'?seed=42');
 check(await mobile.locator('body').evaluate(b=>b.classList.contains('mobile')),'Standard Pages build enables mobile controls');
 await context.close();
 const xrContext=await browser.newContext();const headset=await xrContext.newPage();
 await headset.addInitScript(()=>{window.xrProbes=0;Object.defineProperty(navigator,'xr',{configurable:true,value:{isSessionSupported:async()=>{window.xrProbes++;return true;}}});});
 await ready(headset,base+'?seed=42');check(await headset.evaluate(()=>window.xrProbes)===0,'Standard page never probes XR hardware');
 await ready(headset,base+'webxr/?seed=42');await headset.waitForFunction(()=>!document.getElementById('xr').disabled);
 check(await headset.locator('#xr').textContent()==='Enter WebXR','A supported XR browser gets the headset entry action');
 await xrContext.close();check(errors.length===0,errors.join('\n'));
 await fs.writeFile(new URL('pages-test.json',out),JSON.stringify({checks,base,errors},null,2));
 console.log(`${checks} Pages deployment checks passed.`);
}finally{await browser.close();}
