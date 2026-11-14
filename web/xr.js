// MIT. Native GL rendering is reused per eye; no second simulation or camera mode.
mergeInto(LibraryManager.library, {
  $WanderingXR__postset: 'globalThis.WanderingXR = WanderingXR;',
  $WanderingXR: {
    async start(module,canvas) {
      if(!navigator.xr)throw new Error('WebXR unavailable');
      const session=await navigator.xr.requestSession('immersive-vr',{optionalFeatures:['local-floor']});
      const gl=module.ctx||GL.currentContext.GLctx;
      await gl.makeXRCompatible();
      const layer=new XRWebGLLayer(session,gl);session.updateRenderState({baseLayer:layer});
      let space;try{space=await session.requestReferenceSpace('local-floor');}catch{space=await session.requestReferenceSpace('local');}
      const projection=module._malloc(64),view=module._malloc(64);
      module.ccall('wc_web_action',null,['number','number','number','number'],[100,0,0,0]);
      session.addEventListener('select',()=>module.ccall('wc_web_action',null,['number','number','number','number'],[-1,0,0,0]));
      session.addEventListener('squeezestart',()=>module.ccall('wc_web_action',null,['number','number','number','number'],[-9,0,0,0]));
      session.addEventListener('end',()=>{module._free(projection);module._free(view);module.ccall('wc_web_move',null,['number','number'],[0,0]);module.ccall('wc_xr_end',null,[],[]);});
      const frame=(time,xrframe)=>{
        session.requestAnimationFrame(frame);const pose=xrframe.getViewerPose(space);if(!pose)return;
        let forward=0,side=0;for(const input of session.inputSources){const axes=input.gamepad?.axes;if(axes?.length>=2){const offset=axes.length>=4?2:0;side+=axes[offset];forward-=axes[offset+1];}}
        module.ccall('wc_web_move',null,['number','number'],[Math.abs(forward)>.15?forward:0,Math.abs(side)>.15?side:0]);
        module.ccall('wc_xr_frame',null,[],[]);
        gl.bindFramebuffer(gl.FRAMEBUFFER,layer.framebuffer);
        for(const eye of pose.views){const vp=layer.getViewport(eye);HEAPF32.set(eye.projectionMatrix,projection>>2);HEAPF32.set(eye.transform.inverse.matrix,view>>2);module.ccall('wc_xr_view',null,['number','number','number','number','number','number'],[projection,view,vp.x,vp.y,vp.width,vp.height]);}
        gl.bindFramebuffer(gl.FRAMEBUFFER,null);
      };session.requestAnimationFrame(frame);
    }
  }
});
