	uint8_t u8Array[4];
	
	//TRx_TX_PN9_Enable
	TRx_READREG(0xA028, u8Array);
	OR_U8_ARRAY(u8Array, 0x07, 0x00, 0x00, 0x00);
	TRx_WRITEREG(0xA028, u8Array);

	//TX_PN9_Disable
	TRx_READREG(0xA028, u8Array);
	AND_U8_ARRAY(u8Array, 0xF8, 0xFF, 0xFF, 0xFF);
	TRx_WRITEREG(0xA028, u8Array);

	//RX_PN9_Enable
	SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x02, 0xF0);
	TRx_WRITEREG(0xb20c, u8Array);

	//RX_PN9_Disable
	SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
	TRx_WRITEREG(0xb20c, u8Array);
