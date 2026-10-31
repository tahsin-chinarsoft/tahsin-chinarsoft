(async function(id){
  const S=ms=>new Promise(r=>setTimeout(r,ms));
  const C=JSON.parse(localStorage.__cc); const text=C[id];
  for(let i=0;i<30 && document.querySelector('ytd-watch-flexy')?.getAttribute('video-id')!==id;i++) await S(500);
  document.querySelector('video')?.pause();
  let ph=null;
  for(let i=0;i<40 && !ph;i++){ const c=document.querySelector('ytd-comments#comments'); const top=c?c.getBoundingClientRect().top+window.scrollY:0; if(i%4===3){window.scrollTo(0,0);} else if(top>300){window.scrollTo(0,top-200);} else {window.scrollBy(0,600);} await S(800); ph=document.querySelector('ytd-comments #simplebox-placeholder'); }
  if(ph) ph.scrollIntoView({block:'center'});
  if(!ph) return id+': no-comment-box';
  await S(2000);
  const threads=()=>[...document.querySelectorAll('ytd-comments ytd-comment-thread-renderer')];
  const hasPinned=threads().some(t=>t.innerText.includes('tarafından sabitlendi'));
  if(threads().some(t=>t.innerText.includes(text.slice(0,40)))) return id+': already';
  ph.click(); await S(1200);
  const box=document.querySelector('ytd-comments #contenteditable-root'); box.focus(); document.execCommand('insertText',false,text); await S(600);
  (document.querySelector('ytd-comments #submit-button button')||document.querySelector('ytd-comments #submit-button')).click();
  let mine=null; for(let i=0;i<15&&!mine;i++){ await S(700); mine=threads().find(t=>t.innerText.includes(text.slice(0,40))); }
  if(!mine) return id+': post-unconfirmed';
  if(hasPinned) return id+': posted (existing pin kept)';
  mine.querySelector('#action-menu button').click(); await S(1200);
  const item=[...document.querySelectorAll('ytd-menu-navigation-item-renderer, ytd-menu-service-item-renderer')].find(e=>e.offsetParent&&e.innerText.trim()==='Sabitle');
  if(!item) return id+': posted, no pin item';
  (item.querySelector('tp-yt-paper-item')||item).click(); await S(1500);
  const btn=[...document.querySelectorAll('tp-yt-paper-dialog button')].find(b=>b.offsetParent&&b.innerText.trim()==='Sabitle');
  if(!btn) return id+': posted, no confirm';
  btn.click(); await S(2500);
  const ok=threads().find(t=>t.innerText.includes(text.slice(0,40)))?.innerText.includes('tarafından sabitlendi');
  return id+(ok?': posted+pinned':': posted, pin unconfirmed');
})
