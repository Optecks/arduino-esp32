#include "Update.h"
#include "Arduino.h"
#include "esp_spi_flash.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "esp_task_wdt.h"
#include "esp_flash_encrypt.h"

static const char * _err2str(uint8_t _error){
    if(_error == UPDATE_ERROR_OK){
        return ("No Error");
    } else if(_error == UPDATE_ERROR_WRITE){
        return ("Flash Write Failed");
    } else if(_error == UPDATE_ERROR_ERASE){
        return ("Flash Erase Failed");
    } else if(_error == UPDATE_ERROR_READ){
        return ("Flash Read Failed");
    } else if(_error == UPDATE_ERROR_SPACE){
        return ("Not Enough Space");
    } else if(_error == UPDATE_ERROR_SIZE){
        return ("Bad Size Given");
    } else if(_error == UPDATE_ERROR_STREAM){
        return ("Stream Read Timeout");
    } else if(_error == UPDATE_ERROR_MD5){
        return ("MD5 Check Failed");
    } else if(_error == UPDATE_ERROR_MAGIC_BYTE){
        return ("Wrong Magic Byte");
    } else if(_error == UPDATE_ERROR_ACTIVATE){
        return ("Could Not Activate The Firmware");
    } else if(_error == UPDATE_ERROR_NO_PARTITION){
        return ("Partition Could Not be Found");
    } else if(_error == UPDATE_ERROR_BAD_ARGUMENT){
        return ("Bad Argument");
    } else if(_error == UPDATE_ERROR_ABORT){
        return ("Aborted");
    }
    return ("UNKNOWN");
}

static bool _partitionIsBootable(const esp_partition_t* partition){
    uint8_t buf[32];
    if(!partition){
		Serial.println("#1");
        return false;
    }
	auto err = spi_flash_read_encrypted(partition->address, buf, 32);
	if(err != ESP_OK){
		Serial.println("#2");
		Serial.println(err);		
        return false;
    }

    if(buf[0] != ESP_IMAGE_HEADER_MAGIC) {
		Serial.println("#3");
		Serial.println(buf[0], HEX);
        return false;
    }
    return true;
}

static bool _enablePartition(const esp_partition_t* partition, uint8_t* _first_bytes){
    uint8_t buf[32];
    if(!partition){
        return false;
    }
	auto err_spi_flash_read_encrypted =  spi_flash_read(partition->address, buf, 32);
	Serial.println(err_spi_flash_read_encrypted);
    if(err_spi_flash_read_encrypted != ESP_OK) {
        return false;
    }
    for(size_t i = 0; i< 16; ++i){
		buf[i] = _first_bytes[i];
	}
	if(esp_flash_encryption_enabled()){
		auto err = spi_flash_write(partition->address, buf, 32);
		return err == ESP_OK;
	}else{
		return false;
	}
}

UpdateClass::UpdateClass()
: _error(0)
, _buffer(0)
, _bufferLen(0)
, _size(0)
, _progress_callback(NULL)
, _progress(0)
, _command(U_FLASH)
, _partition(NULL)
{
}

UpdateClass& UpdateClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
    return *this;
}

void UpdateClass::_reset() {
    if (_buffer)
        delete[] _buffer;
    _buffer = 0;
    _bufferLen = 0;
    _progress = 0;
    _size = 0;
    _command = U_FLASH;
}

bool UpdateClass::canRollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition);
}

bool UpdateClass::rollBack(){
    if(_buffer){ //Update is running
        return false;
    }
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    return _partitionIsBootable(partition) && !esp_ota_set_boot_partition(partition);
}

bool UpdateClass::begin(size_t size, int command) {
    if(_size > 0){
        log_w("already running");
        return false;
    }

    _reset();
    _error = 0;

    if(size == 0) {
        _error = UPDATE_ERROR_SIZE;
        return false;
    }

    if (command == U_FLASH) {
        _partition = esp_ota_get_next_update_partition(NULL);
        if(!_partition){
            _error = UPDATE_ERROR_NO_PARTITION;
            return false;
        }
        log_d("OTA Partition: %s", _partition->label);
    }
    else if (command == U_SPIFFS) {
        _partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
        if(!_partition){
            _error = UPDATE_ERROR_NO_PARTITION;
            return false;
        }
    }
    else {
        _error = UPDATE_ERROR_BAD_ARGUMENT;
        log_e("bad command %u", command);
        return false;
    }

    if(size == UPDATE_SIZE_UNKNOWN){
        size = _partition->size;
    } else if(size > _partition->size){
        _error = UPDATE_ERROR_SIZE;
        log_e("too large %u > %u", size, _partition->size);
        return false;
    }

    //initialize
    _buffer = (uint8_t*)malloc(SPI_FLASH_SEC_SIZE);
    if(!_buffer){
        log_e("malloc failed");
        return false;
    }
    _size = size;
    _command = command;
    _md5.begin();
    return true;
}

void UpdateClass::_abort(uint8_t err){
    _reset();
    _error = err;
}

void UpdateClass::abort(){
    _abort(UPDATE_ERROR_ABORT);
}

bool UpdateClass::_writeBuffer(){
    //first bytes of new firmware
    if(!_progress && _command == U_FLASH){
        //check magic
        /*if(_buffer[0] != ESP_IMAGE_HEADER_MAGIC){
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }*/
		for(size_t i = 0; i < 16; ++i){
			_first_bytes[i] = _buffer[i];
		}
        //remove magic byte from the firmware now and write it upon success
        //this ensures that partially written firmware will not be bootable
        for(size_t i = 0; i < 16; ++i){
			_buffer[i] = 0xFF;
		}
    }
    if(spi_flash_erase_sector((_partition->address + _progress)/SPI_FLASH_SEC_SIZE) != ESP_OK){
        _abort(UPDATE_ERROR_ERASE);
        return false;
    }
	if(spi_flash_write(_partition->address + _progress, _buffer, _bufferLen) != ESP_OK){
        _abort(UPDATE_ERROR_WRITE);
        return false;
    }
    //restore magic or md5 will fail
    if(!_progress && _command == U_FLASH){
        //_buffer[0] = ESP_IMAGE_HEADER_MAGIC;
		for(size_t i = 0; i< 16; ++i){
			_buffer[i] = _first_bytes[i];
		}
    }
    _md5.add(_buffer, _bufferLen);
    _progress += _bufferLen;
    _bufferLen = 0;
    return true;
}

bool UpdateClass::_verifyHeader(uint8_t data) {
    if(_command == U_FLASH) {
        if(data != ESP_IMAGE_HEADER_MAGIC) {
            _abort(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        return true;
    } else if(_command == U_SPIFFS) {
        return true;
    }
    return false;
}

bool UpdateClass::_verifyEnd() {
    if(_command == U_FLASH) {
        if(!_enablePartition(_partition,_first_bytes)) {
			Serial.println("!_enablePartition");
            _abort(UPDATE_ERROR_READ);
            return false;
        }
		
		if(!_partitionIsBootable(_partition)){
			Serial.println("!_partitionIsBootable");
			_abort(UPDATE_ERROR_READ);
            return false;
		}

        if(esp_ota_set_boot_partition(_partition)){
            _abort(UPDATE_ERROR_ACTIVATE);
            return false;
        }
        _reset();
        return true;
    } else if(_command == U_SPIFFS) {
        _reset();
        return true;
    }
    return false;
}

bool UpdateClass::setMD5(const char * expected_md5){
    if(strlen(expected_md5) != 32)
    {
        return false;
    }
    _target_md5 = expected_md5;
    return true;
}

bool UpdateClass::end(bool evenIfRemaining){
    if(hasError() || _size == 0){
        return false;
    }

    if(!isFinished() && !evenIfRemaining){
        log_e("premature end: res:%u, pos:%u/%u\n", getError(), progress(), _size);
        _abort(UPDATE_ERROR_ABORT);
        return false;
    }

    if(evenIfRemaining) {
        if(_bufferLen > 0) {
            _writeBuffer();
        }
        _size = progress();
    }

    _md5.calculate();
    if(_target_md5.length()) {
        if(_target_md5 != _md5.toString()){
            _abort(UPDATE_ERROR_MD5);
            return false;
        }
    }

    return _verifyEnd();
}

size_t UpdateClass::write(uint8_t *data, size_t len) {
    if(hasError() || !isRunning()){
        return 0;
    }

    if(len > remaining()){
        _abort(UPDATE_ERROR_SPACE);
        return 0;
    }

    size_t left = len;

    while((_bufferLen + left) > SPI_FLASH_SEC_SIZE) {
        size_t toBuff = SPI_FLASH_SEC_SIZE - _bufferLen;
        memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
        _bufferLen += toBuff;
        if(!_writeBuffer()){
            return len - left;
        }
        left -= toBuff;
    }
    memcpy(_buffer + _bufferLen, data + (len - left), left);
    _bufferLen += left;
    if(_bufferLen == remaining()){
        if(!_writeBuffer()){
            return len - left;
        }
    }
    return len;
}

size_t UpdateClass::writeStream(Stream &data) {
    size_t written = 0;
    size_t toRead = 0;
    if(hasError() || !isRunning())
        return 0;

    /*if(!_verifyHeader(data.peek())) {
        _reset();
        return 0;
    }*/
    if (_progress_callback) {
        _progress_callback(0, _size);
    }
    while(remaining()) {
        toRead = data.readBytes(_buffer + _bufferLen,  (SPI_FLASH_SEC_SIZE - _bufferLen));
        if(toRead == 0) { //Timeout
            delay(1500);
            toRead = data.readBytes(_buffer + _bufferLen, (SPI_FLASH_SEC_SIZE - _bufferLen));
            if(toRead == 0) { //Timeout
				delay(1500);
				toRead = data.readBytes(_buffer + _bufferLen, (SPI_FLASH_SEC_SIZE - _bufferLen));
                if(toRead == 0) {
					_abort(UPDATE_ERROR_STREAM);
					return written;
				}
            }
        }
        _bufferLen += toRead;
        if((_bufferLen == remaining() || _bufferLen == SPI_FLASH_SEC_SIZE) && !_writeBuffer())
            return written;
        written += toRead;
        if(_progress_callback) {
            _progress_callback(_progress, _size);
        }		
		esp_task_wdt_reset();
    }
    if(_progress_callback) {
        _progress_callback(_size, _size);
    }
    return written;
}

void UpdateClass::printError(Stream &out){
    out.println(_err2str(_error));
}

UpdateClass Update;
