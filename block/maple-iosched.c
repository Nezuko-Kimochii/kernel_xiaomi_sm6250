/*
 * MAPLE I/O scheduler stub
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct maple_data {
	struct list_head queue;
};

static void maple_merged_requests(struct request_queue *q,
				 struct request *rq, struct request *next)
{
	list_del_init(&next->queuelist);
}

static int maple_dispatch(struct request_queue *q, int force)
{
	struct maple_data *md = q->elevator->elevator_data;
	struct request *rq;

	rq = list_first_entry_or_null(&md->queue, struct request, queuelist);
	if (rq) {
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}

	return 0;
}

static void maple_add_request(struct request_queue *q, struct request *rq)
{
	struct maple_data *md = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &md->queue);
}

static struct request *maple_former_request(struct request_queue *q,
					 struct request *rq)
{
	struct maple_data *md = q->elevator->elevator_data;

	if (rq->queuelist.prev == &md->queue)
		return NULL;
	return list_prev_entry(rq, queuelist);
}

static struct request *maple_latter_request(struct request_queue *q,
					 struct request *rq)
{
	struct maple_data *md = q->elevator->elevator_data;

	if (rq->queuelist.next == &md->queue)
		return NULL;
	return list_next_entry(rq, queuelist);
}

static int maple_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct maple_data *md;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	md = kmalloc_node(sizeof(*md), GFP_KERNEL, q->node);
	if (!md) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = md;

	INIT_LIST_HEAD(&md->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void maple_exit_queue(struct elevator_queue *e)
{
	struct maple_data *md = e->elevator_data;

	BUG_ON(!list_empty(&md->queue));
	kfree(md);
}

static struct elevator_type elevator_maple = {
	.ops.sq = {
		.elevator_merge_req_fn = maple_merged_requests,
		.elevator_dispatch_fn = maple_dispatch,
		.elevator_add_req_fn = maple_add_request,
		.elevator_former_req_fn = maple_former_request,
		.elevator_latter_req_fn = maple_latter_request,
		.elevator_init_fn = maple_init_queue,
		.elevator_exit_fn = maple_exit_queue,
	},
	.elevator_name = "maple",
	.elevator_owner = THIS_MODULE,
};

static int __init maple_init(void)
{
	return elv_register(&elevator_maple);
}

static void __exit maple_exit(void)
{
	elv_unregister(&elevator_maple);
}

module_init(maple_init);
module_exit(maple_exit);

MODULE_AUTHOR("Kernel stub");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MAPLE I/O scheduler stub");
