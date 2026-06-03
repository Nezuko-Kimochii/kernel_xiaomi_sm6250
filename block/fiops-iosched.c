/*
 * FIOPS I/O scheduler stub
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct fiops_data {
	struct list_head queue;
};

static void fiops_merged_requests(struct request_queue *q,
				 struct request *rq, struct request *next)
{
	list_del_init(&next->queuelist);
}

static int fiops_dispatch(struct request_queue *q, int force)
{
	struct fiops_data *fd = q->elevator->elevator_data;
	struct request *rq;

	rq = list_first_entry_or_null(&fd->queue, struct request, queuelist);
	if (rq) {
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}

	return 0;
}

static void fiops_add_request(struct request_queue *q, struct request *rq)
{
	struct fiops_data *fd = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &fd->queue);
}

static struct request *fiops_former_request(struct request_queue *q,
					 struct request *rq)
{
	struct fiops_data *fd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &fd->queue)
		return NULL;
	return list_prev_entry(rq, queuelist);
}

static struct request *fiops_latter_request(struct request_queue *q,
					 struct request *rq)
{
	struct fiops_data *fd = q->elevator->elevator_data;

	if (rq->queuelist.next == &fd->queue)
		return NULL;
	return list_next_entry(rq, queuelist);
}

static int fiops_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct fiops_data *fd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	fd = kmalloc_node(sizeof(*fd), GFP_KERNEL, q->node);
	if (!fd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = fd;

	INIT_LIST_HEAD(&fd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void fiops_exit_queue(struct elevator_queue *e)
{
	struct fiops_data *fd = e->elevator_data;

	BUG_ON(!list_empty(&fd->queue));
	kfree(fd);
}

static struct elevator_type elevator_fiops = {
	.ops.sq = {
		.elevator_merge_req_fn = fiops_merged_requests,
		.elevator_dispatch_fn = fiops_dispatch,
		.elevator_add_req_fn = fiops_add_request,
		.elevator_former_req_fn = fiops_former_request,
		.elevator_latter_req_fn = fiops_latter_request,
		.elevator_init_fn = fiops_init_queue,
		.elevator_exit_fn = fiops_exit_queue,
	},
	.elevator_name = "fiops",
	.elevator_owner = THIS_MODULE,
};

static int __init fiops_init(void)
{
	return elv_register(&elevator_fiops);
}

static void __exit fiops_exit(void)
{
	elv_unregister(&elevator_fiops);
}

module_init(fiops_init);
module_exit(fiops_exit);

MODULE_AUTHOR("Kernel stub");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FIOPS I/O scheduler stub");
