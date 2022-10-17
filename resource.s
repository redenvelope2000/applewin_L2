        .data

.balign 4                         /* Word alignment */
.global _binary_resource_zip_start
.global __binary_resource_zip_start
__binary_resource_zip_start:                      
_binary_resource_zip_start:                      
.incbin "resource.zip"         

.global _binary_resource_zip_end
.global __binary_resource_zip_end
__binary_resource_zip_end:
_binary_resource_zip_end:


