<?php
/*
 * $RCSfile: AdminUsers.inc,v $
 *
 * Gallery - a web based photo album viewer and editor
 * Copyright (C) 2000-2006 Bharat Mediratta
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA.
 */
/**
 * @version $Revision: 1.34 $ $Date: 2006/01/17 01:52:01 $
 * @package GalleryCore
 * @subpackage UserInterface
 * @author Bharat Mediratta <bharat@menalto.com>
 */

/**
 * This controller will send the user to the chosen subView in the AdminUsers View
 *
 * @package GalleryCore
 * @subpackage UserInterface
 */
class AdminUsersController extends GalleryController {

    /**
     * @see GalleryController::handleRequest
     */
    function handleRequest($form) {
	global $gallery;

	$ret = GalleryCoreApi::assertUserIsSiteAdministrator();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$status = array();
	$error = array();
	$results = array();
	$user = null;
	if (!empty($form['text']['userName'])) {
	    list ($ret, $user) =
		GalleryCoreApi::fetchUserByUserName($form['text']['userName']);
	    if ($ret) {
		if (!($ret->getErrorCode() & ERROR_MISSING_OBJECT)) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}
	    }
	}

	if (isset($form['action']['filterClear'])) {

	    /* Clear the filter */
	    GalleryUtilities::putRequestVariable('form[list][filter]', null);

	} else if (isset($form['action']['create'])) {

	    /* Show the "create user" view */
	    $redirect['view'] = 'core.SiteAdmin';
	    $redirect['subView'] = 'core.AdminCreateUser';

	} else if (isset($form['action']['editFromText'])) {

	    if (empty($form['text']['userName'])) {
		$error[] = 'form[error][text][noUserSpecified]';
	    } else if ($user == null) {
		$error[] = 'form[error][text][noSuchUser]';
	    } else {
		/* Show the "delete user" view */
		$redirect['view'] = 'core.SiteAdmin';
		$redirect['userId'] = $user->getId();
		$redirect['subView'] = 'core.AdminEditUser';
	    }

	} else if (isset($form['action']['deleteFromText'])) {

	    if (empty($form['text']['userName'])) {
		$error[] = 'form[error][text][noUserSpecified]';
	    } else if ($user == null) {
		$error[] = 'form[error][text][noSuchUser]';
	    } else {

		/*
		 * Check to see if we're trying to delete the anonymous user, or
		 * ourself (can't do either of those).
		 */
		list ($ret, $anonymousUserId) =
		    GalleryCoreApi::getPluginParameter('module', 'core', 'id.anonymousUser');
		if ($ret) {
		    return array($ret->wrap(__FILE__, __LINE__), null);
		}

		if ($user->getId() == $anonymousUserId) {
		    $error[] = 'form[error][text][cantDeleteAnonymous]';
		}

		if ($user->getId() == $gallery->getActiveUserId()) {
		    $error[] = 'form[error][text][cantDeleteSelf]';
		}
	    }

	    if (empty($error)) {
		/* Show the "delete user" view */
		$redirect['view'] = 'core.SiteAdmin';
		$redirect['subView'] = 'core.AdminDeleteUser';
		$redirect['userId'] = $user->getId();
	    }
	}

	if (!empty($redirect)) {
	    $results['redirect'] = $redirect;
	} else {
	    $results['delegate']['view'] = 'core.SiteAdmin';
	    $results['delegate']['subView'] = 'core.AdminUsers';
	}
	$results['status'] = $status;
	$results['error'] = $error;

	return array(null, $results);
    }
}

/**
 * This view will show available options to administer the users of Gallery
 *
 * @package GalleryCore
 * @subpackage UserInterface
 */
class AdminUsersView extends GalleryView {

    /**
     * @see GalleryView::loadTemplate
     */
    function loadTemplate(&$template, &$form) {
	global $gallery;

	$ret = GalleryCoreApi::assertUserIsSiteAdministrator();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	if ($form['formName'] != 'AdminUsers') {
	    /* Set some defaults */
	    $form['text']['userName'] = '';
	    $form['formName'] = 'AdminUsers';
	}

	if (!isset($form['list']['filter'])) {
	    $form['list']['filter'] = '';
	}

	if (!isset($form['list']['page']) || $form['list']['page'] < 1) {
	    $form['list']['page'] = 1;
	}

	/* Fetch the user count every time we reload */
	list ($ret, $totalUserCount) = GalleryCoreApi::fetchUserCount();
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$form['list']['count'] = $totalUserCount;
	$form['list']['pageSize'] = $totalUserCount > 10 ? 10 : $totalUserCount + 2;

	/* If we have a filter, find out how many users match it */
	if (!empty($form['list']['filter'])) {
	    list ($ret, $form['list']['count']) =
		GalleryCoreApi::fetchUserCount($form['list']['filter']);
	    if ($ret) {
		return array($ret->wrap(__FILE__, __LINE__), null);
	    }
	}

	/* Figure out our max pages, make sure our current page fits in it */
	$form['list']['maxPages'] = ceil($form['list']['count'] / $form['list']['pageSize']);
	if ($form['list']['page'] > $form['list']['maxPages']) {
	    $form['list']['page'] = $form['list']['maxPages'];
	}

	/* Calculate the next/back pages */
	$form['list']['nextPage'] = min($form['list']['page']+1, $form['list']['maxPages']);
	$form['list']['backPage'] = max(1, $form['list']['page']-1);

	list ($ret, $userNames) =
	    GalleryCoreApi::fetchUserNames($form['list']['pageSize'],
					   ($form['list']['page'] - 1) * $form['list']['pageSize'],
					   $form['list']['filter']);
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	list ($ret, $anonymousUserId) =
	    GalleryCoreApi::getPluginParameter('module', 'core', 'id.anonymousUser');
	if ($ret) {
	    return array($ret->wrap(__FILE__, __LINE__), null);
	}

	$myUserId = $gallery->getActiveUserId();

	$form['list']['userNames'] = array();
	foreach ($userNames as $userId => $userName) {
	    $form['list']['userNames'][$userId]['userName'] = $userName;
	    if ($userId == $anonymousUserId || $userId == $myUserId) {
		$form['list']['userNames'][$userId]['can']['delete'] = false;
	    } else {
		$form['list']['userNames'][$userId]['can']['delete'] = true;
	    }
	}

	$AdminUsers = array();
	$AdminUsers['totalUserCount'] = $totalUserCount;

	$template->setVariable('AdminUsers', $AdminUsers);
	$template->setVariable('controller', 'core.AdminUsers');
	return array(null,
		     array('body' => 'modules/core/templates/AdminUsers.tpl'));
    }
}
?>
