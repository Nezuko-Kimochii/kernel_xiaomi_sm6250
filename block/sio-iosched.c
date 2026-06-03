/*
 * SIO I/O scheduler stub
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct sio_data {
	struct list_head queue;
};

static void sio_merged_requests(struct request_queue *q,
				 struct request *rq, struct request *next)
{
	list_del_init(&next->queuelist);
}

static int sio_dispatch(struct request_queue *q, int force)
{
	struct sio_data *sd = q->elevator->elevator_data;
	struct request *rq;

	rq = list_first_entry_or_null(&sd->queue, struct request, queuelist);
	if (rq) {
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}

	return 0;
}

static void sio_add_request(struct request_queue *q, struct request *rq)
{
	struct sio_data *sd = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &sd->queue);
}

static struct request *sio_former_request(struct request_queue *q,
					 struct request *rq)
{
	struct sio_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &sd->queue)
		return NULL;
	return list_prev_entry(rq, queuelist);
}

static struct request *sio_latter_request(struct request_queue *q,
					 struct request *rq)
{
	struct sio_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.next == &sd->queue)
		return NULL;
	return list_next_entry(rq, queuelist);
}

static int sio_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct sio_data *sd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	sd = kmalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
	if (!sd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = sd;

	INIT_LIST_HEAD(&sd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void sio_exit_queue(struct elevator_queue *e)
{
	struct sio_data *sd = e->elevator_data;

	BUG_ON(!list_empty(&sd->queue));
	kfree(sd);
}

static struct elevator_type elevator_sio = {
	.ops.sq = {
		.elevator_merge_req_fn = sio_merged_requests,
		.elevator_dispatch_fn = sio_dispatch,
		.elevator_add_req_fn = sio_add_request,
		.elevator_former_req_fn = sio_former_request,
		.elevator_latter_req_fn = sio_latter_request,
		.elevator_init_fn = sio_init_queue,
		.elevator_exit_fn = sio_exit_queue,
	},
	.elevator_name = "sio",
	.elevator_owner = THIS_MODULE,
};

static int __init sio_init(void)
{
	return elv_register(&elevator_sio);
}

static void __exit sio_exit(void)
{
	elv_unregister(&elevator_sio);
}

module_init(sio_init);
module_exit(sio_exit);

MODULE_AUTHOR("Kernel stub");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SIO I/O scheduler stub");
