mod bindings {
    #![allow(non_upper_case_globals)]
    #![allow(non_camel_case_types)]
    #![allow(non_snake_case)]
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

use std::collections::HashMap;
use std::{ffi::CString, ptr::null, ptr::null_mut};

pub enum WuServeTimeout {
    Block,
    Immediate,
    BlockForMilliseconds(i32),
}

#[derive(Debug)]
pub struct WuHost {
    host: *mut bindings::WuHost,
    client_pointers_to_ids: HashMap<*mut bindings::WuClient, u32>,
    client_ids_to_pointers: HashMap<u32, *mut bindings::WuClient>,
    client_id_counter: u32,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Ord, PartialOrd)]
pub struct WuClient(u32);

#[derive(Clone, Debug)]
pub enum WuEvent {
    ClientJoin(WuClient),
    ClientLeave(WuClient),
    TextData(WuClient, String),
    BinaryData(WuClient, Vec<u8>),
}

impl WuHost {
    pub fn create(host_addr: &str, host_port: &str, max_clients: i32) -> Self {
        let host_addr = CString::new(host_addr).unwrap();
        let host_port = CString::new(host_port).unwrap();
        let mut host = WuHost {
            host: null_mut(),
            client_pointers_to_ids: HashMap::new(),
            client_ids_to_pointers: HashMap::new(),
            client_id_counter: 0,
        };

        let result = unsafe {
            bindings::WuHostCreate(
                host_addr.as_ptr(),
                host_port.as_ptr(),
                max_clients,
                &mut host.host,
            )
        };
        assert!(result == 0);

        host
    }

    pub fn serve(&mut self, timeout: WuServeTimeout) -> Option<WuEvent> {
        let timeout_int: i32 = match timeout {
            WuServeTimeout::Block => -1,
            WuServeTimeout::Immediate => 0,
            WuServeTimeout::BlockForMilliseconds(n) => n,
        };

        let mut evt = bindings::WuEvent {
            type_: bindings::WuEventType_WuEvent_BinaryData,
            client: null_mut(),
            data: null(),
            length: 0,
        };

        if unsafe { bindings::WuHostServe(self.host, &mut evt, timeout_int) } != 1 {
            None
        } else {
            let client_id = if self.client_pointers_to_ids.contains_key(&evt.client) {
                self.client_pointers_to_ids[&evt.client]
            } else {
                self.client_id_counter += 1;
                self.client_pointers_to_ids
                    .insert(evt.client, self.client_id_counter);
                self.client_ids_to_pointers
                    .insert(self.client_id_counter, evt.client);
                self.client_id_counter
            };

            let wu_client = WuClient(client_id);

            match evt.type_ {
                bindings::WuEventType_WuEvent_ClientJoin => Some(WuEvent::ClientJoin(wu_client)),
                bindings::WuEventType_WuEvent_ClientLeave => {
                    self.remove_client(wu_client);
                    Some(WuEvent::ClientLeave(wu_client))
                }
                bindings::WuEventType_WuEvent_TextData => {
                    let slice =
                        unsafe { std::slice::from_raw_parts(evt.data, evt.length as usize) };
                    Some(WuEvent::TextData(
                        wu_client,
                        String::from_utf8(slice.to_vec()).unwrap(),
                    ))
                }
                bindings::WuEventType_WuEvent_BinaryData => {
                    let slice =
                        unsafe { std::slice::from_raw_parts(evt.data, evt.length as usize) };
                    Some(WuEvent::BinaryData(wu_client, slice.to_vec()))
                }
                _ => panic!(),
            }
        }
    }

    pub fn remove_client(&mut self, client: WuClient) {
        let WuClient(client_id) = client;
        if !self.client_ids_to_pointers.contains_key(&client_id) {
            return;
        }

        let client_ptr = self.client_ids_to_pointers[&client_id];
        self.client_ids_to_pointers.remove(&client_id);
        self.client_pointers_to_ids.remove(&client_ptr);

        unsafe {
            bindings::WuHostRemoveClient(self.host, client_ptr);
        }
    }

    pub fn send_text(&self, client: WuClient, text: &str) {
        let WuClient(client_id) = client;
        if !self.client_ids_to_pointers.contains_key(&client_id) {
            return;
        }
        let client_ptr = self.client_ids_to_pointers[&client_id];

        let c_text = CString::new(text).unwrap();
        unsafe {
            bindings::WuHostSendText(self.host, client_ptr, c_text.as_ptr(), text.len() as i32);
        }
    }

    pub fn send_binary(&self, client: WuClient, data: &[u8]) {
        let WuClient(client_id) = client;
        if !self.client_ids_to_pointers.contains_key(&client_id) {
            return;
        }
        let client_ptr = self.client_ids_to_pointers[&client_id];

        unsafe {
            bindings::WuHostSendBinary(self.host, client_ptr, data.as_ptr(), data.len() as i32);
        }
    }

    pub fn set_error_callback(&self) {
        unimplemented!()
    }

    pub fn find_client(&self) {
        unimplemented!()
    }
}

impl Drop for WuHost {
    fn drop(&mut self) {
        unsafe {
            bindings::WuHostDestroy(self.host);
        }
    }
}
