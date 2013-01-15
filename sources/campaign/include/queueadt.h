// =========
// Queue ADT
// =========

// -----------------
// User Descriptions
// -----------------

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ QueueElement CreateQueueElement (void)                              บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Creates and returns a Queue Element.                                บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void DisposeQueueElement (QueueElement E)                           บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Disposes a Queue Element.                                           บ
   บ                                                                     บ
   บ WARNING: Does NOT dispose of any User Data attached to the Element. บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void SetQueueElementUserData (QueueElement E, void* UserDataPointer)บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Allows User Defined Data to be attached to a Queue Element.         บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void* GetQueueElementUserData (QueueElement E)                      บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Allows access to any User Defined Data previously attached to the   บ
   บ given Queue Element.                                                บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ Queue CreateQueue (void)                                            บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Creates an "empty" Queue.                                           บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void EnqueueElement (Queue Q, QueueElement E)                       บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Enqueues a Queue Element onto a Queue.                              บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ QueueElement DequeueElement (Queue Q)ement E)                       บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Dequeues and returns a Queue Element from a Queue.                  บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

 /*ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
   บ void DisposeQueue (Queue Q)                                         บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Disposes a Queue and all Queue Elements contained within that Queue.บ
   บ                                                                     บ
   บ WARNING: Does NOT dispose of any User Data attached to the Elements บ
   บ          contained in the Queue.                                    บ
   วฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ
   บ Author: Gary L. Stottlemyer      ณ Inquiries: Gary L. Stottlemyer   บ
   บ Project: Falcon 4.0              ณ Current Project: Falcon 4.0      บ
   ศออออออออออออออออออออออออออออออออออฯออออออออออออออออออออออออออออออออออผ*/

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

   typedef void* QueueElement;
   typedef void* Queue;

   extern QueueElement CreateQueueElement (void);

   extern void DisposeQueueElement (QueueElement E);

   extern void SetQueueElementUserData (QueueElement E, void* UserDataPointer);

   extern void* GetQueueElementUserData (QueueElement E);

   extern Queue CreateQueue (void);

   extern void EnqueueElement (Queue Q, QueueElement E);

   extern QueueElement DequeueElement (Queue Q);

   extern void DisposeQueue (Queue Q);
