/* BEETLE portable encrypted container V2. Public shared key, not secrecy protection.
   Binary layout: 8 byte ASCII BEETLE01; 0x02; 12-byte CSPRNG nonce;
   AES-256-GCM ciphertext plus 16-byte authenticated tag. 21-byte AAD.
   The compatibility key ships inside both offline HTML apps. */
const BeetleFileV2=(()=>{
 const magic=Uint8Array.from([66,69,69,84,76,69,48,49]), version=2;
 // Distribution-wide compatibility key. NOT a private user secret.
 const publicKey=Uint8Array.from([0x71,0x12,0xd2,0x3a,0xbe,0x7c,0x81,0x36,0x67,0xa3,0x01,0xdf,0x4e,0x94,0x70,0xc8,0x2d,0x51,0x9f,0x09,0xd5,0x82,0x13,0x4a,0xbf,0x72,0x68,0xec,0x04,0xca,0x3d,0xf9]);
 let keyPromise;
 function ready(){if(!globalThis.crypto?.subtle||!globalThis.crypto?.getRandomValues)throw Error('AES encryption needs HTTPS or localhost and WebCrypto.');}
 function key(){ready();return keyPromise ||= crypto.subtle.importKey('raw',publicKey,{name:'AES-GCM'},false,['encrypt','decrypt']);}
 function signature(bytes){return bytes.length>=9 && magic.every((v,i)=>bytes[i]===v) ? bytes[8] : -1;}
 async function encrypt(bytes){ready();if(!(bytes instanceof Uint8Array))bytes=new Uint8Array(bytes);if(bytes.length<1||bytes.length>2097152)throw Error('Project exceeds file limit.');const iv=crypto.getRandomValues(new Uint8Array(12)),head=Uint8Array.from([...magic,version,...iv]);const data=new Uint8Array(await crypto.subtle.encrypt({name:'AES-GCM',iv,additionalData:head,tagLength:128},await key(),bytes));return new Blob([head,data],{type:'application/octet-stream'});}
 async function decrypt(file){ready();if(file.size<38||file.size>2097200)throw Error('Invalid .beetle file size.');const bytes=new Uint8Array(await file.arrayBuffer());if(signature(bytes)!==version)throw Error('Not a password-free BEETLE V2 file.');const head=bytes.subarray(0,21);try{return new Uint8Array(await crypto.subtle.decrypt({name:'AES-GCM',iv:bytes.subarray(9,21),additionalData:head,tagLength:128},await key(),bytes.subarray(21)));}catch{throw Error('This .beetle file is damaged or failed integrity verification.');}}
 return {encrypt,decrypt,signature,version};
})();
