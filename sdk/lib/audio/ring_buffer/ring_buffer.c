#include "basic_include.h"
#include "lib/audio/ring_buffer/ring_buffer.h"
#include "lib/heap/av_psram_heap.h"

RINGBUF *ringbuf_Init(uint8_t elementsize, uint32_t elementcount)
{
    RINGBUF *ringbuf = (RINGBUF*)av_psram_zalloc(sizeof(RINGBUF));
    if(ringbuf == NULL) {
        return NULL;
	}
    ringbuf->front = 0;
    ringbuf->rear = 0;
    ringbuf->elementsize = elementsize;
    ringbuf->elementcount = elementcount + 1;
    ringbuf->data = av_psram_malloc(elementsize * (elementcount + 1) * sizeof(uint8_t));
    if(ringbuf->data == NULL) {
        av_psram_free(ringbuf);
        ringbuf = NULL;
    }
    return ringbuf;
}

int32_t ringbuf_read_available(RINGBUF *ringbuf)
{
    if(ringbuf == NULL) {
        return 0;
    } 
    return ((ringbuf->rear + ringbuf->elementcount - ringbuf->front) % ringbuf->elementcount);
}

int32_t ringbuf_write_available(RINGBUF *ringbuf)
{
    if(ringbuf == NULL) {
        return 0;
    } 
	return ((ringbuf->front + ringbuf->elementcount - ringbuf->rear - 1) % ringbuf->elementcount);   
}

int32_t ringbuf_read(RINGBUF *ringbuf, void *data, uint32_t elementcount)
{
    uint32_t readcount = 0;
    if(ringbuf == NULL) {
        return 0;
    }  
    if(ringbuf->front == ringbuf->rear) {
        return 0;        
    }
    if(elementcount > (uint32_t)ringbuf_read_available(ringbuf)) {
        readcount = (uint32_t)ringbuf_read_available(ringbuf);
	}
    else {
        readcount = elementcount;
	}
    if((ringbuf->front + readcount) > (ringbuf->elementcount))
	{
		os_memcpy((uint8_t*)data, ringbuf->data + ringbuf->front * ringbuf->elementsize, 
                 (ringbuf->elementcount - ringbuf->front) * ringbuf->elementsize);
		os_memcpy(((uint8_t*)data) + (ringbuf->elementcount - ringbuf->front) * ringbuf->elementsize, 
                 ringbuf->data, (readcount - ringbuf->elementcount + ringbuf->front) * ringbuf->elementsize);		
	}
	else {
		os_memcpy((uint8_t*)data, ringbuf->data + ringbuf->front * ringbuf->elementsize, readcount * ringbuf->elementsize);	
	}
	ringbuf->front = (ringbuf->front + readcount) % (ringbuf->elementcount);
    return readcount;
}

int32_t ringbuf_write(RINGBUF *ringbuf, void *data, uint32_t elementcount)
{
    uint32_t writecount = 0;
    if(ringbuf == NULL) {
        return 0;
    }
    if(elementcount > (uint32_t)ringbuf_write_available(ringbuf)) {
        writecount = (uint32_t)ringbuf_write_available(ringbuf);
	}
    else {
        writecount = elementcount;
	}
	if((ringbuf->rear + writecount) > (ringbuf->elementcount))
	{
		os_memcpy(ringbuf->data + ringbuf->rear * ringbuf->elementsize, (uint8_t*)data, 
                 (ringbuf->elementcount - ringbuf->rear) * ringbuf->elementsize);
		os_memcpy(ringbuf->data, ((uint8_t*)data) + (ringbuf->elementcount - ringbuf->rear) * ringbuf->elementsize, 
                 (writecount - ringbuf->elementcount + ringbuf->rear) * ringbuf->elementsize);
	}
	else {
		os_memcpy(ringbuf->data + ringbuf->rear * ringbuf->elementsize, (uint8_t*)data, writecount * ringbuf->elementsize);
	}
    ringbuf->rear = (ringbuf->rear + writecount) % (ringbuf->elementcount);  
    return writecount; 
}

int32_t ringbuf_move_readptr(RINGBUF *ringbuf, int32_t elementcount)
{
    uint32_t read_avable = ringbuf_read_available(ringbuf);
    if(read_avable < elementcount) {
        ringbuf->front = (ringbuf->front + read_avable) % (ringbuf->elementcount);
        return read_avable;
    }
    else {
        ringbuf->front = (ringbuf->front + elementcount) % (ringbuf->elementcount);
        return elementcount;
    }
}

int32_t ringbuf_read_nomove(RINGBUF *ringbuf, void *data, uint32_t elementcount)
{
    uint32_t readcount = 0;
    if(ringbuf == NULL) {
        return 0;
    }  
    if(ringbuf->front == ringbuf->rear) {
        return 0;        
    }
    if(elementcount > (uint32_t)ringbuf_read_available(ringbuf)) {
        readcount = (uint32_t)ringbuf_read_available(ringbuf);
	}
    else {
        readcount = elementcount;
	}
    if((ringbuf->front + readcount) > (ringbuf->elementcount))
	{
		os_memcpy((uint8_t*)data, ringbuf->data + ringbuf->front * ringbuf->elementsize, 
                 (ringbuf->elementcount - ringbuf->front) * ringbuf->elementsize);
		os_memcpy(((uint8_t*)data) + (ringbuf->elementcount - ringbuf->front) * ringbuf->elementsize, 
                 ringbuf->data, (readcount - ringbuf->elementcount + ringbuf->front) * ringbuf->elementsize);		
	}
	else {
		os_memcpy((uint8_t*)data, ringbuf->data + ringbuf->front * ringbuf->elementsize, readcount * ringbuf->elementsize);	
	}
    return readcount;    
}

void ringbuf_clean(RINGBUF *ringbuf)
{
    ringbuf->front = 0;
    ringbuf->rear = 0;    
}

void ringbuf_del(RINGBUF *ringbuf)
{
    if(ringbuf == NULL) {
        return;
    } 
    av_psram_free(ringbuf->data);
    av_psram_free(ringbuf);
}